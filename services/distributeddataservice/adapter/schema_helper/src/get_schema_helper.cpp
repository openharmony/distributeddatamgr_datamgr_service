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

#define LOG_TAG "SchemaHelper"
#include "get_schema_helper.h"

#include "bundle_mgr_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "resource_manager.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace DistributedData {
using namespace OHOS::Global::Resource;

sptr<AppExecFwk::IBundleMgr> GetSchemaHelper::GetBundleMgr()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (object_ != nullptr) {
        return iface_cast<AppExecFwk::IBundleMgr>(object_);
    }
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return nullptr;
    }
    object_ = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (object_ == nullptr) {
        ZLOGE("BMS service not ready to complete.");
        return nullptr;
    }
    deathRecipient_ = new (std::nothrow) GetSchemaHelper::ServiceDeathRecipient(weak_from_this());
    if (deathRecipient_ == nullptr) {
        ZLOGE("deathRecipient alloc failed.");
        object_ = nullptr;
        return nullptr;
    }
    if (!object_->AddDeathRecipient(deathRecipient_)) {
        ZLOGE("add death recipient failed.");
        object_ = nullptr;
        deathRecipient_ = nullptr;
        return nullptr;
    }
    return iface_cast<AppExecFwk::IBundleMgr>(object_);
}
std::vector<std::string> GetSchemaHelper::GetSchemaFromHap(const std::string &schemaPath, const AppInfo &info)
{
    std::vector<std::string> schemas;
    auto bmsClient = GetBundleMgr();
    if (bmsClient == nullptr) {
        ZLOGE("GetBundleMgr is nullptr!");
        return schemas;
    }
    OHOS::AppExecFwk::BundleInfo bundleInfo;
    int32_t flag = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE);
    auto ret = bmsClient->GetCloneBundleInfo(info.bundleName, flag, info.appIndex, bundleInfo, info.userId);
    if (ret != ERR_OK) {
        ZLOGE("GetCloneBundleInfo failed. errCode:%{public}d", ret);
        return schemas;
    }

    std::shared_ptr<ResourceManager> resMgr(CreateResourceManager());
    if (resMgr == nullptr) {
        ZLOGE("resMgr is nullptr.");
        return schemas;
    }
    for (auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        resMgr->AddResource(hapModuleInfo.hapPath.c_str());
        size_t length = 0;
        std::unique_ptr<uint8_t[]> fileContent;
        auto ret = resMgr->GetRawFileFromHap(schemaPath, length, fileContent);
        if (ret != ERR_OK) {
            ZLOGE("GetRawFileFromHap failed. bundleName:%{public}s ret:%{public}d", info.bundleName.c_str(), ret);
            continue;
        }
        std::string schema(fileContent.get(), fileContent.get() + length);
        schemas.emplace_back(schema);
    }
    return schemas;
}
void GetSchemaHelper::OnRemoteDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ZLOGE("remote object died, object=null ? %{public}s.", object_ == nullptr ? "true" : "false");
    if (object_ != nullptr) {
        object_->RemoveDeathRecipient(deathRecipient_);
    }
    object_ = nullptr;
    deathRecipient_ = nullptr;
}

GetSchemaHelper::~GetSchemaHelper()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (object_ != nullptr) {
        object_->RemoveDeathRecipient(deathRecipient_);
    }
}

GetSchemaHelper &GetSchemaHelper::GetInstance()
{
    static GetSchemaHelper helper;
    return helper;
}
} // namespace DistributedData
} // namespace OHOS