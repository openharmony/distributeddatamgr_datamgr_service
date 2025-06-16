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

#ifndef DATASHARESERVICE_PROXY_DATA_MANAGER_H
#define DATASHARESERVICE_PROXY_DATA_MANAGER_H

#include "bundle_mgr_proxy.h"
#include "dataproxy_handle_common.h"
#include "db_delegate.h"

namespace OHOS::DataShare {

class ProxyDataNode final : public VersionData {
public:
    ProxyDataNode();
    ProxyDataNode(const SerialDataShareProxyData proxyData, const std::string bundleName,
        const int32_t userId, const int32_t &appIndex, const uint32_t tokenId)
        : VersionData(0), proxyData(proxyData), bundleName(bundleName),
        userId(userId), appIndex(appIndex), tokenId(tokenId) {}
    ~ProxyDataNode() = default;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    SerialDataShareProxyData proxyData;
    std::string bundleName;
    int32_t userId = Id::INVALID_USER;
    int32_t appIndex = 0;
    uint32_t tokenId;
};

class PublishedProxyData final : public KvData {
public:
    explicit PublishedProxyData(const ProxyDataNode &node);
    ~PublishedProxyData() = default;
    static int32_t Query(const std::string &uri, const BundleInfo &callerBundleInfo, DataShareProxyData &proxyData);
    static int32_t Delete(const std::string &uri, const BundleInfo &callerBundleInfo, DataShareProxyData &oldProxyData,
        DataShareObserver::ChangeType &type, bool isInstallEvent = false);
    static int32_t Upsert(const DataShareProxyData &proxyData, const BundleInfo &callerBundleInfo,
        DataShareObserver::ChangeType &type, bool isInstallEvent = false);
    static bool VerifyPermission(const BundleInfo &callerBundleInfo, const ProxyDataNode &data);
    static bool VerifySelfAccess(const BundleInfo &callerBundleInfo,
        const ProxyDataNode &data, bool isInstallEvent = false);
    bool HasVersion() const override;
    int GetVersion() const override;
    std::string GetValue() const override;

private:
    static bool CheckAllowList(const std::string callerBundleName, DataShareProxyData proxyData);
    ProxyDataNode value;
};

class ProxyDataManager {
public:
    static ProxyDataManager &GetInstance();
    void OnAppInstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId);
    void OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId);

private:
    bool GetCrossAppSharedConfig(const std::string &bundleName, int32_t user, int32_t index,
        std::vector<DataShareProxyData> &proxyDatas);
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_PROXY_DATA_MANAGER_H
