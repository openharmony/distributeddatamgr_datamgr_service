/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DATASHARESERVICE_BUNDLEMGR_PROXY_H
#define DATASHARESERVICE_BUNDLEMGR_PROXY_H

#include <memory>
#include <string>
#include <utility>

#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_proxy.h"
#include "concurrent_map.h"
#include "data_share_profile_config.h"
#include "lru_bucket.h"

namespace OHOS::DataShare {
struct ProfileConfig {
    ProfileInfo profile;
    int resultCode = 0;
};

struct ProxyData {
    std::string uri;
    std::string requiredReadPermission;
    std::string requiredWritePermission;
    ProfileConfig profileInfo;
};

struct CrossAppSharedConfig {
    std::string resourcePath;
    ProxyDataProfileInfo profile;
};

struct HapModuleInfo {
    std::string resourcePath;
    std::string hapPath;
    std::string moduleName;
    std::vector<ProxyData> proxyDatas;
    CrossAppSharedConfig crossAppSharedConfig;
};

struct ExtensionAbilityInfo {
    AppExecFwk::ExtensionAbilityType type = AppExecFwk::ExtensionAbilityType::UNSPECIFIED;
    std::string readPermission;
    std::string writePermission;
    std::string uri;
    std::string resourcePath;
    std::string hapPath;
    ProfileConfig profileInfo;
};

struct BundleConfig {
    std::string appIdentifier;
    std::string name;
    bool singleton = false;
    bool isSystemApp = false;
    std::vector<HapModuleInfo> hapModuleInfos;
    std::vector<ExtensionAbilityInfo> extensionInfos;
};

struct BundleInfo {
    std::string bundleName;
    std::string appIdentifier;
    int32_t userId;
    int32_t appIndex;
    uint32_t tokenId;
};

struct SilentBundleInfo {
    std::string bundleName;
    int32_t userId;
    SilentBundleInfo(const std::string &name, int32_t userId) : bundleName(name), userId(userId) {}
    bool operator<(const SilentBundleInfo &other) const
    {
        if (bundleName == other.bundleName) {
        return userId < other.userId;
        }
        return bundleName < other.bundleName;
    }
};

class BundleMgrProxy final : public std::enable_shared_from_this<BundleMgrProxy> {
public:
    BundleMgrProxy() = default;
    ~BundleMgrProxy();
    static std::shared_ptr<BundleMgrProxy> GetInstance();
    int GetBundleInfoFromBMS(const std::string &bundleName, int32_t userId,
        BundleConfig &bundleConfig, int32_t appIndex = 0);
    int GetBundleInfoFromBMSWithCheck(const std::string &bundleName, int32_t userId,
        BundleConfig &bundleConfig, int32_t appIndex = 0);
    std::pair<int, bool> IsConfigSilentProxy(const std::string &bundleName, int32_t userId,
        const std::string &storeName);
    void Delete(const std::string &bundleName, int32_t userId, int32_t appIndex);
    sptr<IRemoteObject> CheckBMS();
    std::pair<int, std::string> GetCallerAppIdentifier(const std::string &bundleName, int32_t userId);
private:
    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ServiceDeathRecipient(std::weak_ptr<BundleMgrProxy> owner) : owner_(owner) {}
        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            auto owner = owner_.lock();
            if (owner != nullptr) {
                owner->OnProxyDied();
            }
        }

    private:
        std::weak_ptr<BundleMgrProxy> owner_;
    };
    sptr<AppExecFwk::BundleMgrProxy> GetBundleMgrProxy();
    void OnProxyDied();
    std::pair<int, BundleConfig> ConvertToDataShareBundle(AppExecFwk::BundleInfo &bundleInfo);
    std::pair<int, std::vector<ExtensionAbilityInfo>> ConvertExtensionAbility(AppExecFwk::BundleInfo &bundleInfo);
    std::pair<int, std::vector<HapModuleInfo>> ConvertHapModuleInfo(AppExecFwk::BundleInfo &bundleInfo);
    void UpdateSilentConfig(const SilentBundleInfo &silentBundleInfo, const std::string &storeName,
        bool isSilent);
    std::mutex mutex_;
    sptr<IRemoteObject> proxy_;
    sptr<BundleMgrProxy::ServiceDeathRecipient> deathRecipient_;
    ConcurrentMap<std::string, BundleConfig> bundleCache_;
    ConcurrentMap<std::string, std::string> callerInfoCache_;
    static constexpr const char *DATA_SHARE_EXTENSION_META = "ohos.extension.dataShare";
    static constexpr const char *DATA_SHARE_PROPERTIES_META = "dataProperties";
    static constexpr size_t CACHE_SIZE = 32;
    LRUBucket<SilentBundleInfo, std::map<std::string, bool>> isSilent_ {CACHE_SIZE};
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
