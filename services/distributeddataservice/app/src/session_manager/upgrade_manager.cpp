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
#include "executor_factory.h"
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

void UpgradeManager::Init()
{
    OHOS::DistributedKv::KvStoreTask retryTask([this]() {
        do {
            if (InitLocalCapability()) {
                break;
            }
            static constexpr int RETRY_INTERVAL = 500; // millisecond
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_INTERVAL));
        } while (true);
    });
    ExecutorFactory::GetInstance().Execute(std::move(retryTask));
}

CapMetaData UpgradeManager::GetCapability(const std::string &deviceId, bool &status)
{
    status = true;
    if (capabilityMap_.Contains(deviceId)) {
        return capabilityMap_.Find(deviceId).second;
    }
    ZLOGI("load capability from meta");
    CapMetaData capMetaData;
    auto dbKey = CapMetaRow::GetKeyFor(deviceId);
    ZLOGD("cap key:%{public}s", Anonymous::Change(std::string(dbKey.begin(), dbKey.end())).c_str());
    status = MetaDataManager::GetInstance().LoadMeta(std::string(dbKey.begin(), dbKey.end()), capMetaData);
    if (status) {
        capabilityMap_.Insert(deviceId, capMetaData);
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
        capabilityMap_.Insert(localDeviceId, capMetaData);
    }
    ZLOGI("put capability meta data ret %{public}d", status);
    return status;
}

void UpgradeManager::SetCompatibleIdentifyByType(DistributedDB::KvStoreNbDelegate *storeDelegate,
    const KvStoreTuple &tuple, DistributedData::AUTH_GROUP_TYPE groupType)
{
    if (storeDelegate == nullptr) {
        ZLOGE("null store delegate");
        return;
    }
    auto localDevice = DmAdapter::GetInstance().GetLocalDevice().uuid;
    auto devices =
        AuthDelegate::GetInstance()->GetTrustedDevicesByType(groupType, std::stoi(tuple.userId), tuple.appId);
    auto result = std::remove_if(devices.begin(), devices.end(), [&localDevice](const std::string &device) {
        if (localDevice == device) {
            return true;
        }
        bool flag = false;
        auto capability = DistributedData::UpgradeManager::GetInstance().GetCapability(device, flag);
        return !flag || capability.version >= DistributedData::CapMetaData::CURRENT_VERSION;
    });
    devices.erase(result, devices.end());

    bool isSuccess = false;
    auto compatibleUser = UpgradeManager::GetIdentifierByType(groupType, isSuccess);
    if (!isSuccess) {
        ZLOGW("get identifier by type failed");
        return;
    }

    auto syncIdentifier =
        DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(compatibleUser, tuple.appId, tuple.storeId);
    ZLOGI("set compatible identifier, store:%{public}s, user:%{public}s, device:%{public}.10s",
        Anonymous::Change(tuple.storeId).c_str(), compatibleUser.c_str(),
        DistributedData::Serializable::Marshall(devices).c_str());
    storeDelegate->SetEqualIdentifier(syncIdentifier, devices);
}

std::string UpgradeManager::GetIdentifierByType(int32_t groupType, bool &isSuccess)
{
    isSuccess = true;
    if (groupType == PEER_TO_PEER_GROUP) {
        return "default";
    } else if (groupType == IDENTICAL_ACCOUNT_GROUP) {
        auto accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
        if (accountId.empty()) {
            ZLOGE("failed to get current account id");
            isSuccess = false;
            return {};
        }
        return accountId;
    } else {
        ZLOGW("not supported group type:%{public}d", groupType);
        isSuccess = false;
        return {};
    }
}
} // namespace OHOS::DistributedData
