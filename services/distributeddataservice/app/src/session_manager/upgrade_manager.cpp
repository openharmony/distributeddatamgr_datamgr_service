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

void UpgradeManager::SetCompatibleIdentifyByType(DistributedDB::KvStoreNbDelegate *storeDelegate,
    const KvStoreTuple &tuple)
{
    static constexpr int32_t NO_ACCOUNT = 0;
    static constexpr int32_t IDENTICAL_ACCOUNT = 1;
    if (storeDelegate == nullptr) {
        ZLOGE("null store delegate");
        return;
    }

    auto devices = DmAdapter::ToUUID(DmAdapter::GetInstance().GetRemoteDevices());
    if (devices.empty()) {
        ZLOGI("no remote devs");
        return;
    }

    std::vector<std::string> accDevs {};
    std::vector<std::string> defaultAccDevs {};
    std::string accDevsId = "";
    std::string defaultDevsId = "";
    for (const auto &devid : devices) {
        if (DmAdapter::GetInstance().IsOHOSType(devid)) {
            continue;
        }
        auto netType = DmAdapter::GetInstance().GetAccountType(devid);
        if (netType == IDENTICAL_ACCOUNT) {
            accDevsId = tuple.userId;
            accDevs.push_back(devid);
        } else if (netType == NO_ACCOUNT) {
            defaultDevsId = "default";
            defaultAccDevs.push_back(devid);
        }
    }
    if (!accDevs.empty()) {
        auto syncIdentifier =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(accDevsId, tuple.appId, tuple.storeId);
        ZLOGI("account set compatible identifier, store:%{public}s, user:%{public}s, device:%{public}.10s",
            Anonymous::Change(tuple.storeId).c_str(), accDevsId.c_str(),
            DistributedData::Serializable::Marshall(accDevs).c_str());
        storeDelegate->SetEqualIdentifier(syncIdentifier, accDevs);
    }
    if (!defaultAccDevs.empty()) {
        auto syncIdentifier =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(defaultDevsId, tuple.appId, tuple.storeId);
        ZLOGI("no account set compatible identifier, store:%{public}s, device:%{public}.10s",
            Anonymous::Change(tuple.storeId).c_str(), DistributedData::Serializable::Marshall(accDevs).c_str());
        storeDelegate->SetEqualIdentifier(syncIdentifier, defaultAccDevs);
    }
}
} // namespace OHOS::DistributedData
