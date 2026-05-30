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

enum ProxyDataUpsertMode {
    NORMAL_PUBLISH,
    PUBLISH_MULTIVALUES,
    PUT_MULTIVALUES,
    REMOVE_MULTIVALUES
};

enum QueryMode {
    QUERY_VALUE,      // Query non-appendable data, returns error if isMultiValues=true
    QUERY_MULTIVALUES,  // Query appendable data, returns error if isMultiValues=false
    ANY_QUERY          // Query any type (default for Subscribe etc.)
};

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

struct UpsertContext {
    std::shared_ptr<KvDBDelegate> delegate;
    ProxyDataNode oldData;
    BundleInfo callerBundleInfo;
};

/**
 * Result structure for AppendData operation.
 * Contains error code and assigned index (only valid when error code is SUCCESS).
 */
struct AppendResult {
    int32_t errorCode;      // Error code from DataProxyErrorCode enum
    int32_t assignedIndex;  // Assigned index, only valid when errorCode == SUCCESS, otherwise -1
    AppendResult(int32_t code, int32_t index) : errorCode(code), assignedIndex(index) {}
};

class PublishedProxyData final : public KvData {
public:
    explicit PublishedProxyData(const ProxyDataNode &node);
    ~PublishedProxyData() = default;
    static int32_t Query(const std::string &uri, const BundleInfo &callerBundleInfo,
        DataShareProxyData &proxyData, QueryMode mode = ANY_QUERY);
    static int32_t Delete(const std::string &uri, const BundleInfo &callerBundleInfo, DataShareProxyData &oldProxyData,
        DataShareObserver::ChangeType &type);
    static int32_t Upsert(const DataShareProxyData &proxyData, const BundleInfo &callerBundleInfo,
        DataShareObserver::ChangeType &type, const DataProxyConfig &proxyConfig,
        const ProxyDataUpsertMode mode = NORMAL_PUBLISH);
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
    static bool CheckAndCorrectProxyData(DataShareProxyData &proxyData, const DataProxyConfig &proxyConfig,
        const ProxyDataUpsertMode mode = NORMAL_PUBLISH);
    static int32_t UpdateProxyDataList(std::shared_ptr<KvDBDelegate> delegate, const std::string &uri,
        const BundleInfo &callerBundleInfo);
    static int32_t UpsertInsert(std::shared_ptr<KvDBDelegate> delegate,
        const DataShareProxyData &data, const BundleInfo &callerBundleInfo,
        DataShareObserver::ChangeType &type, const ProxyDataUpsertMode mode);
    static int32_t UpsertUpdate(UpsertContext &ctx, const DataShareProxyData &data,
        const ProxyDataUpsertMode mode, DataShareObserver::ChangeType &type);
    static int32_t UpsertRemoveValue(UpsertContext &ctx, const std::string &key,
        DataShareObserver::ChangeType &type);
    static int32_t UpsertPutValues(UpsertContext &ctx, const std::string &key,
        const DataProxyValue &value, DataShareObserver::ChangeType &type);
    static size_t CalculateCurrentTotal(const ProxyDataNode &oldData);
    static int32_t CheckValueAndTotalLimits(const DataProxyValue &value,
        size_t currentTotal, size_t maxValueLength);
    static int32_t UpsertMultiValueData(const UpsertContext &ctx, const std::string &key,
        const DataProxyValue &value, DataShareObserver::ChangeType &type);
    static DataShareProxyData BuildMultiValuesAfterPut(const ProxyDataNode &existing,
        const std::string &key, const DataProxyValue &value, const BundleInfo &callerBundleInfo);
    static DataShareProxyData BuildMultiValuesAfterRemove(const ProxyDataNode &existing,
        const std::string &key, const BundleInfo &callerBundleInfo);
    static bool IsConfigCompatible(const ProxyDataUpsertMode mode, bool existingIsMultiValues);
    static bool CanInsertMultiValues(const BundleInfo &callerBundleInfo, const ProxyDataNode &data);
    static bool CanModifyMultiValue(const ProxyDataNode &data, const std::string &key,
        const BundleInfo &callerBundleInfo);
    static int32_t BuildInitMultiValues(DataShareProxyData &data, const std::string &appIdentifier);
    ProxyDataNode value;
    static std::mutex mutex_;
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