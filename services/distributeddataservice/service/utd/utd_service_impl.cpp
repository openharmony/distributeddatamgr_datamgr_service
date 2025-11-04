/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define LOG_TAG "UtdServiceImpl"

#include "utd_service_impl.h"

#include "ipc_skeleton.h"
#include "tokenid_kit.h"
#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_proxy.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"
#include "utd_cfgs_checker.h"

namespace OHOS {
namespace UDMF {
using namespace Security::AccessToken;
using namespace OHOS::DistributedData;
using FeatureSystem = DistributedData::FeatureSystem;

constexpr const char *MANAGE_DYNAMIC_UTD_TYPE = "ohos.permission.MANAGE_DYNAMIC_UTD_TYPE";
constexpr const char *FOUNDATION_PROCESS_NAME = "foundation";

__attribute__((used)) UtdServiceImpl::Factory UtdServiceImpl::factory_;
UtdServiceImpl::Factory::Factory()
{
    ZLOGI("Register utd creator!");
    FeatureSystem::GetInstance().RegisterCreator("utd", [this]() {
        if (product_ == nullptr) {
            product_ = std::make_shared<UtdServiceImpl>();
        }
        return product_;
    }, FeatureSystem::BIND_NOW);
}

UtdServiceImpl::Factory::~Factory()
{
    product_ = nullptr;
}

int32_t UtdServiceImpl::RegServiceNotifier(sptr<IRemoteObject> iUtdNotifier)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    Security::AccessToken::NativeTokenInfo tokenInfo;
    if (Security::AccessToken::AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) == 0) {
        if (tokenInfo.processName == FOUNDATION_PROCESS_NAME) {
            foundationTokenId_ = tokenId;
        }
    }
    utdNotifiers_.InsertOrAssign(tokenId, iface_cast<UtdNotifierProxy>(iUtdNotifier));
    ZLOGI("utdNotifiers_.Size = %{public}zu", utdNotifiers_.Size());
    return E_OK;
}

int32_t UtdServiceImpl::RegisterTypeDescriptors(const std::vector<TypeDescriptorCfg> &descriptors)
{
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    if (!TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx)) {
        ZLOGE("No system permission");
        return E_NO_SYSTEM_PERMISSION;
    }
    if (!VerifyPermission(MANAGE_DYNAMIC_UTD_TYPE, IPCSkeleton::GetCallingTokenID())) {
        ZLOGE("No manageDynamicUtdType permission");
        return E_NO_PERMISSION;
    }
    if (!UtdCfgsChecker::GetInstance().CheckTypeCfgsFormat({descriptors, {}})) {
        ZLOGE("CheckTypeCfgsFormat not pass");
        return E_FORMAT_ERROR;
    }

    if (foundationTokenId_ == 0) {
        ZLOGE("foundationTokenId_ not exits");
            return E_ERROR;
    }
    auto [exist, notifier] = utdNotifiers_.Find(foundationTokenId_);
    if (!exist || notifier == nullptr) {
        ZLOGE("foundation notifier not found");
        return E_ERROR;
    }

    std::string bundleName;
    uint32_t callingTokenId = IPCSkeleton::GetCallingTokenID();
    if (GetHapBundleNameByToken(callingTokenId, bundleName) == false) {
        ZLOGE("GetHapBundleNameByToken failed");
        return E_ERROR;
    }
    auto status = notifier->RegisterDynamicUtd(descriptors, bundleName);
    if (status != E_OK) {
        ZLOGE("Failed, status: %{public}d", status);
        return status;
    }

    return NotifyUtdClients(callingTokenId);
}

int32_t UtdServiceImpl::UnregisterTypeDescriptors(const std::vector<std::string> &typeIds)
{
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    if (!TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx)) {
        ZLOGE("No system permission");
        return E_NO_SYSTEM_PERMISSION;
    }
    if (!VerifyPermission(MANAGE_DYNAMIC_UTD_TYPE, IPCSkeleton::GetCallingTokenID())) {
        ZLOGE("No manageDynamicUtdType permission");
        return E_NO_PERMISSION;
    }
    if (!UtdCfgsChecker::GetInstance().CheckTypeIdsFormat(typeIds)) {
        ZLOGE("CheckTypeIdsFormat not pass");
        return E_INVALID_TYPE_ID;
    }

    if (foundationTokenId_ == 0) {
        ZLOGE("foundationTokenId_ not exits");
            return E_ERROR;
    }
    auto [exist, notifier] = utdNotifiers_.Find(foundationTokenId_);
    if (!exist || notifier == nullptr) {
        ZLOGE("foundation notifier not found");
        return E_ERROR;
    }

    std::string bundleName;
    uint32_t callingTokenId = IPCSkeleton::GetCallingTokenID();
    if (GetHapBundleNameByToken(callingTokenId, bundleName) == false) {
        ZLOGE("GetHapBundleNameByToken failed");
        return E_ERROR;
    }
    auto status = notifier->UnregisterDynamicUtd(typeIds, bundleName);
    if (status != E_OK) {
        ZLOGE("Failed, status: %{public}d", status);
        return status;
    }

    return NotifyUtdClients(callingTokenId);
}

int32_t UtdServiceImpl::NotifyUtdClients(uint32_t callingTokenId)
{
    ZLOGI("utdNotifiers_.Size = %{public}zu", utdNotifiers_.Size());
    ExecutorPool::TaskId taskId = executors_->Execute([this, callingTokenId] {
        utdNotifiers_.EraseIf([this, callingTokenId](auto tokenId, const sptr<UtdNotifierProxy>& notifier) {
            if (tokenId == foundationTokenId_ || tokenId == callingTokenId) {
                return false;
            }
            auto ret = notifier->DynamicUtdChange();
            if (ret != E_OK) {
                ZLOGE("DynamicUtdChange failed, ret: %{public}d", ret);
                return true;
            }
            return false;
        });
    });
    if (taskId == ExecutorPool::INVALID_TASK_ID) {
        ZLOGE("Task execution failed");
        return E_ERROR;
    }
    return E_OK;
}

int32_t UtdServiceImpl::OnBind(const BindInfo &bindInfo)
{
    executors_ = bindInfo.executors;
    return 0;
}

bool UtdServiceImpl::GetHapBundleNameByToken(uint32_t tokenId, std::string &bundleName)
{
    Security::AccessToken::HapTokenInfo hapInfo;
    if (Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo)
        == Security::AccessToken::AccessTokenKitRet::RET_SUCCESS) {
        bundleName = hapInfo.bundleName;
        return true;
    }
    ZLOGE("Get bundle name faild");
    return false;
}

bool UtdServiceImpl::VerifyPermission(const std::string &permission, uint32_t callerTokenId)
{
    if (permission.empty()) {
        ZLOGE("VerifyPermission failed, Permission is empty.");
        return false;
    }
    int status = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerTokenId, permission);
    if (status != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        ZLOGE("Permission denied. status:%{public}d, token:%{public}u, permission:%{public}s",
            status, callerTokenId, permission.c_str());
        return false;
    }
    return true;
}
} // namespace UDMF
} // namespace OHOS