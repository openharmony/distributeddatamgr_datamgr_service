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

#define LOG_TAG "PermitDelegate"
#include "permit_delegate.h"
#include "communication_provider.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/strategy_meta_data.h"
#include "permission/permission_validator.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "store_types.h"
#include "runtime_config.h"
#include "log_print.h"

namespace OHOS::DistributedData {
using DBStatus = DistributedDB::DBStatus;
using DBConfig = DistributedDB::RuntimeConfig;
using DBFlag = DistributedDB::PermissionCheckFlag;
using Commu = OHOS::AppDistributedKv::CommunicationProvider;
using PermissionValidator = OHOS::DistributedKv::PermissionValidator;

PermitDelegate::PermitDelegate()
{}

PermitDelegate::~PermitDelegate()
{}

PermitDelegate &PermitDelegate::GetInstance()
{
    static PermitDelegate permit;
    return permit;
}

void PermitDelegate::Init()
{
    auto activeCall = [this](const ActiveParam &param) -> bool {
        return SyncActivate(param);
    };
    DBStatus status = DBConfig::SetSyncActivationCheckCallback(activeCall);
    ZLOGI("set active callback status:%d.", status);

    auto permitCall = [this](const CheckParam &Param, uint8_t flag) -> bool {
        return VerifyPermission(Param, flag);
    };
    status = DBConfig::SetPermissionCheckCallback(permitCall);
    ZLOGI("set permission callback status:%d.", status);

    auto extraCall = [this](const CondParam &param) -> std::map<std::string, std::string> {
        return GetExtraCondition(param);
    };
    status = DBConfig::SetPermissionConditionCallback(extraCall);
    ZLOGI("set extra condition call status:%d.", status);
}

bool PermitDelegate::SyncActivate(const ActiveParam &param)
{
    ZLOGD("user:%{public}s, app:%{public}s, store:%{public}s, instanceId:%{public}d",
        param.userId.c_str(), param.appId.c_str(), Anonymous::Change(param.storeId).c_str(), param.instanceId);
    if (param.instanceId != 0) {
        return false;
    }
    std::set<std::string> activeUsers = UserDelegate::GetInstance().GetLocalUsers();
    return activeUsers.count(param.userId);
}

bool PermitDelegate::VerifyPermission(const CheckParam &param, uint8_t flag)
{
    ZLOGI("user:%{public}s, appId:%{public}s, storeId:%{public}s, remote devId:%{public}s, instanceId:%{public}d,"
          "flag:%{public}u", param.userId.c_str(), param.appId.c_str(), Anonymous::Change(param.storeId).c_str(),
          Anonymous::Change(param.deviceId).c_str(), param.instanceId, flag);

    auto devId = Commu::GetInstance().GetLocalDevice().uuid;
    StoreMetaData data;
    data.user = param.userId == "default" ? DEFAULT_USER : param.userId;
    data.storeId = param.storeId;
    data.deviceId = devId;
    data.instanceId = param.instanceId;
    appId2BundleNameMap_.Compute(param.appId, [&data, &param](const auto &key, std::string &value) {
        if (!value.empty()) {
            data.bundleName = value;
            return true;
        }
        AppIDMetaData appIDMeta;
        MetaDataManager::GetInstance().LoadMeta(key, appIDMeta, true);
        if (appIDMeta.appId == param.appId) {
            data.bundleName = appIDMeta.bundleName;
            value = appIDMeta.bundleName;
        }
        return !value.empty();
    });
    auto key = data.GetKey();
    if (!metaDataBucket_.Get(key, data)) {
        if (!MetaDataManager::GetInstance().LoadMeta(key, data)) {
            ZLOGE("load meta fail");
            return false;
        }
        metaDataBucket_.Set(data.GetKey(), data);
    }
    if (data.appType.compare("default") == 0) {
        ZLOGD("default, sync permission success.");
        return true;
    }
    auto status = VerifyStrategy(data, param.deviceId);
    if (status != Status::SUCCESS) {
        ZLOGE("verify strategy fail, status:%d.", status);
        return false;
    }
    return PermissionValidator::GetInstance().CheckSyncPermission(data.tokenId);
}

bool PermitDelegate::VerifyExtraCondition(const std::map<std::string, std::string> &cond) const
{
    (void)cond;
    return true;
}

std::map<std::string, std::string> PermitDelegate::GetExtraCondition(const CondParam &param)
{
    (void)param;
    return {};
}

Status PermitDelegate::VerifyStrategy(const StoreMetaData &data, const std::string &rmdevId) const
{
    StrategyMeta local(data.deviceId, data.user, data.bundleName, data.storeId);
    MetaDataManager::GetInstance().LoadMeta(local.GetKey(), local);
    StrategyMeta remote(rmdevId, data.user, data.bundleName, data.storeId);
    MetaDataManager::GetInstance().LoadMeta(remote.GetKey(), remote);
    if (!local.IsEffect() || !remote.IsEffect()) {
        ZLOGD("no range, sync permission success.");
        return Status::SUCCESS;
    }
    auto lremotes = local.capabilityRange.remoteLabel;
    auto rlocals = remote.capabilityRange.localLabel;
    for (const auto &lrmote : lremotes) {
        if (std::find(rlocals.begin(), rlocals.end(), lrmote) != rlocals.end()) {
            ZLOGD("find range, sync permission success.");
            return Status::SUCCESS;
        }
    }
    return Status::ERROR;
}

void PermitDelegate::DelCache(const std::string &key)
{
    ZLOGI("meta key: %{public}s", key.c_str());
    metaDataBucket_.Delete(key);
}
} // namespace OHOS::DistributedData
