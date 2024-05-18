/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "UpgradeManager"

#include "upgrade_manager.h"

#include <thread>
#include "account_delegate.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "utils/anonymous.h"
#include "utils/constant.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
using DmAdapter = DistributedData::DeviceManagerAdapter;
UpgradeManager &UpgradeManager::GetInstance()
{
    static UpgradeManager instance;
    return instance;
}

void UpgradeManager::Init(std::shared_ptr<ExecutorPool> executors)
{
    if (executors_) {
        return;
    }
    executors_ = std::move(executors);
    executors_->Execute(GetTask());
}

ExecutorPool::Task UpgradeManager::GetTask()
{
    return [this] {
        auto succ = InitLocalCapability();
        if (succ) {
            return;
        }
        executors_->Schedule(std::chrono::milliseconds(RETRY_INTERVAL), GetTask());
    };
}

CapMetaData UpgradeManager::GetCapability(const std::string &deviceId, bool &status)
{
    status = true;
    auto index = capabilities_.Find(deviceId);
    if (index.first) {
        return index.second;
    }
    ZLOGI("load capability from meta");
    CapMetaData capMetaData;
    auto dbKey = CapMetaRow::GetKeyFor(deviceId);
    ZLOGD("cap key:%{public}s", Anonymous::Change(std::string(dbKey.begin(), dbKey.end())).c_str());
    status = MetaDataManager::GetInstance().LoadMeta(std::string(dbKey.begin(), dbKey.end()), capMetaData);
    if (status) {
        capabilities_.Insert(deviceId, capMetaData);
    }
    ZLOGI("device:%{public}s, version:%{public}d, insert:%{public}d", Anonymous::Change(deviceId).c_str(),
        capMetaData.version, status);
    return capMetaData;
}

bool UpgradeManager::InitLocalCapability()
{
    auto localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    CapMetaData capMetaData;
    capMetaData.version = CapMetaData::CURRENT_VERSION;
    auto dbKey = CapMetaRow::GetKeyFor(localDeviceId);
    bool status = MetaDataManager::GetInstance().SaveMeta({ dbKey.begin(), dbKey.end() }, capMetaData);
    if (status) {
        capabilities_.Insert(localDeviceId, capMetaData);
    }
    ZLOGI("put capability meta data ret %{public}d", status);
    return status;
}

void UpgradeManager::GetIdentifierParams(std::vector<std::string> &devices,
    const std::vector<std::string> &uuids, int32_t authType)
{
    for (const auto &devId : uuids) {
        if (DmAdapter::GetInstance().IsOHOSType(devId)) {
            continue;
        }
        if (DmAdapter::GetInstance().GetAuthType(devId) != authType) {
            continue;
        }
        if (authType == IDENTICAL_ACCOUNT) {
            devices.push_back(devId);
        } else if (authType == NO_ACCOUNT) {
            devices.push_back(devId);
        }
    }
}

void UpgradeManager::SetCompatibleIdentifyByType(DistributedDB::KvStoreNbDelegate *storeDelegate,
    const KvStoreTuple &tuple)
{
    if (storeDelegate == nullptr) {
        ZLOGE("null store delegate");
        return;
    }
    auto uuids = DmAdapter::ToUUID(DmAdapter::GetInstance().GetRemoteDevices());
    if (uuids.empty()) {
        ZLOGI("no remote devs");
        return;
    }

    std::vector<std::string> sameAccountDevs {};
    std::vector<std::string> defaultAccountDevs {};
    GetIdentifierParams(sameAccountDevs, uuids, IDENTICAL_ACCOUNT);
    GetIdentifierParams(defaultAccountDevs, uuids, NO_ACCOUNT);
    if (!sameAccountDevs.empty()) {
        auto syncIdentifier =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(tuple.userId, tuple.appId, tuple.storeId);
        ZLOGI("set compatible identifier store:%{public}s, user:%{public}s, device:%{public}.10s",
            Anonymous::Change(tuple.storeId).c_str(), tuple.userId.c_str(),
            DistributedData::Serializable::Marshall(sameAccountDevs).c_str());
        storeDelegate->SetEqualIdentifier(syncIdentifier, sameAccountDevs);
    }
    if (!defaultAccountDevs.empty()) {
        auto syncIdentifier =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(DEFAULT_ACCOUNTID, tuple.appId, tuple.storeId);
        ZLOGI("set compatible identifier, store:%{public}s,  device:%{public}.10s",
            Anonymous::Change(tuple.storeId).c_str(),
            DistributedData::Serializable::Marshall(defaultAccountDevs).c_str());
        storeDelegate->SetEqualIdentifier(syncIdentifier, defaultAccountDevs);
    }
}
} // namespace OHOS::DistributedData
