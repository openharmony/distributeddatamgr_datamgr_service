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

class ProxyDataListNode final : public VersionData {
public:
    ProxyDataListNode();
    ProxyDataListNode(const std::vector<std::string> &uris, int32_t userId, uint32_t tokenId)
        : VersionData(0), uris(uris), userId(userId), tokenId(tokenId) {}
    ~ProxyDataListNode() = default;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    std::vector<std::string> uris;
    int32_t userId = Id::INVALID_USER;
    uint32_t tokenId = 0;
};

class ProxyDataList final : public KvData {
public:
    explicit ProxyDataList(const ProxyDataListNode &node);
    ~ProxyDataList() = default;
    static int32_t Query(uint32_t tokenId, int32_t userId, std::vector<std::string> &proxyDataList);
    bool HasVersion() const override;
    int GetVersion() const override;
    std::string GetValue() const override;

private:
    ProxyDataListNode value;
};
    
class ProxyDataNode final : public VersionData {
public:
    ProxyDataNode();
    ProxyDataNode(const SerialDataShareProxyData proxyData, int32_t userId, uint32_t tokenId)
        : VersionData(0), proxyData(proxyData), userId(userId), tokenId(tokenId) {}
    ~ProxyDataNode() = default;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    SerialDataShareProxyData proxyData;
    int32_t userId = Id::INVALID_USER;
    uint32_t tokenId = 0;
};

class PublishedProxyData final : public KvData {
public:
    explicit PublishedProxyData(const ProxyDataNode &node);
    ~PublishedProxyData() = default;
    static int32_t Query(const std::string &uri, const BundleInfo &callerBundleInfo, DataShareProxyData &proxyData);
    static int32_t Delete(const std::string &uri, const BundleInfo &callerBundleInfo, DataShareProxyData &oldProxyData,
        DataShareObserver::ChangeType &type);
    static int32_t Upsert(const DataShareProxyData &proxyData, const BundleInfo &callerBundleInfo,
        DataShareObserver::ChangeType &type);
    static bool VerifyPermission(const BundleInfo &callerBundleInfo, const ProxyDataNode &data);
    bool HasVersion() const override;
    int GetVersion() const override;
    std::string GetValue() const override;

private:
    static int32_t InsertProxyData(std::shared_ptr<KvDBDelegate> kvDelegate, const std::string &bundleName,
        const int32_t &user, const uint32_t &tokenId, const DataShareProxyData &proxyData);
    static int32_t UpdateProxyData(std::shared_ptr<KvDBDelegate> kvDelegate, const std::string &bundleName,
        const int32_t &user, const uint32_t &tokenId, const DataShareProxyData &proxyData);
    static int32_t PutIntoTable(std::shared_ptr<KvDBDelegate> kvDelegate, int32_t user,
        uint32_t tokenId, const std::vector<std::string> &proxyDataList, const DataShareProxyData &proxyData);
    static bool CheckAndCorrectProxyData(DataShareProxyData &proxyData);
    ProxyDataNode value;
};

class ProxyDataManager {
public:
    static ProxyDataManager &GetInstance();
    void OnAppInstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId);
    void OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId);
    void OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId);

private:
    bool GetCrossAppSharedConfig(const std::string &bundleName, int32_t user, int32_t index,
        std::vector<DataShareProxyData> &proxyDatas);
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_PROXY_DATA_MANAGER_H