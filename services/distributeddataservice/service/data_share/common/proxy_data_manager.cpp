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

#include <algorithm>
#include <cstdint>
#define LOG_TAG "ProxyDataManager"

#include "datashare_observer.h"
#include "data_share_profile_config.h"
#include "dataproxy_handle_common.h"
#include "log_print.h"
#include "proxy_data_manager.h"
#include "utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
std::mutex PublishedProxyData::mutex_;
ProxyDataManager &ProxyDataManager::GetInstance()
{
    static ProxyDataManager instance;
    return instance;
}

ProxyDataList::ProxyDataList(const ProxyDataListNode &node)
    : KvData(Id(std::to_string(node.tokenId), node.userId)), value(node)
{
}

bool ProxyDataList::HasVersion() const
{
    return true;
}

int ProxyDataList::GetVersion() const
{
    return value.GetVersion();
}

std::string ProxyDataList::GetValue() const
{
    return DistributedData::Serializable::Marshall(value);
}

PublishedProxyData::PublishedProxyData(const ProxyDataNode &node)
    : KvData(Id(node.proxyData.uri, node.userId)), value(node)
{
}

bool PublishedProxyData::HasVersion() const
{
    return true;
}

int PublishedProxyData::GetVersion() const
{
    return value.GetVersion();
}

std::string PublishedProxyData::GetValue() const
{
    return DistributedData::Serializable::Marshall(value);
}

ProxyDataListNode::ProxyDataListNode() : VersionData(-1) {}

ProxyDataNode::ProxyDataNode() : VersionData(-1) {}

bool ProxyDataNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node[GET_NAME(proxyData)], proxyData);
    ret = ret && SetValue(node[GET_NAME(userId)], userId);
    ret = ret && SetValue(node[GET_NAME(tokenId)], tokenId);
    return ret && VersionData::Marshal(node);
}

bool ProxyDataNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(proxyData), proxyData);
    ret = ret && GetValue(node, GET_NAME(userId), userId);
    return ret && GetValue(node, GET_NAME(tokenId), tokenId);
}

bool ProxyDataListNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node[GET_NAME(uris)], uris);
    ret = ret && SetValue(node[GET_NAME(userId)], userId);
    ret = ret && SetValue(node[GET_NAME(tokenId)], tokenId);
    return ret && VersionData::Marshal(node);
}

bool ProxyDataListNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(uris), uris);
    ret = ret && GetValue(node, GET_NAME(userId), userId);
    ret = ret && GetValue(node, GET_NAME(tokenId), tokenId);
    return ret;
}

bool PublishedProxyData::VerifyPermission(const BundleInfo &callerBundleInfo, const ProxyDataNode &data)
{
    // 1. Publisher access: check tokenId
    if (callerBundleInfo.tokenId == data.tokenId) {
        return true;
    }

    // 2. allowList check
    for (const auto &item : data.proxyData.allowList) {
        // 2.1 Global public config check
        if (item == ALLOW_ALL) {
            return true;
        }
        // 2.2 Specific app check
        if (callerBundleInfo.appIdentifier == item) {
            return true;
        }
    }
    return false;
}


/**
 * @brief Get the serialized length of a DataProxyValue.
 * @param value The DataProxyValue to calculate length for.
 * @return The length of value when serialized to string.
 */
static size_t GetValueLength(const DataProxyValue &value)
{
    switch (value.index()) {
        case DataProxyValueType::VALUE_INT: {
            int64_t val = std::get<int64_t>(value);
            return std::to_string(val).size();
        }
        case DataProxyValueType::VALUE_DOUBLE:
            return std::to_string(std::get<double>(value)).size();
        case DataProxyValueType::VALUE_STRING:
            return std::get<std::string>(value).size();
        case DataProxyValueType::VALUE_BOOL: {
            bool val = std::get<bool>(value);
            // boolean value when converted to string will have length of 4 or 5
            return val ? 4 : 5;
        }
        default:
            return 0;
    }
}

static void CheckAndCorrectAppIdentifierList(std::vector<std::string> &list,
    const std::string &fieldName, const std::string &uri)
{
    int32_t count = 1;
    auto it = list.begin();
    while (it != list.end()) {
        if (it->size() > APPIDENTIFIER_MAX_SIZE) {
            ZLOGW("%{public}s appidentifier of proxyData %{public}s is over limit, size %{public}zu",
                fieldName.c_str(), URIUtils::Anonymous(uri).c_str(), it->size());
            it = list.erase(it);
        } else {
            if (count++ > ALLOW_LIST_MAX_COUNT) {
                break;
            }
            it++;
        }
    }
    if (list.size() > static_cast<size_t>(ALLOW_LIST_MAX_COUNT)) {
        ZLOGW("%{public}s of proxyData %{public}s is over limit, size %{public}zu",
            fieldName.c_str(), URIUtils::Anonymous(uri).c_str(), list.size());
        list.resize(static_cast<size_t>(ALLOW_LIST_MAX_COUNT));
    }
}

bool PublishedProxyData::CheckAndCorrectProxyData(DataShareProxyData &proxyData, const DataProxyConfig &proxyConfig,
    const ProxyDataUpsertMode mode)
{
    if (proxyConfig.maxValueLength_ != DataProxyMaxValueLength::MAX_LENGTH_4K &&
        proxyConfig.maxValueLength_ != DataProxyMaxValueLength::MAX_LENGTH_100K) {
        ZLOGE("Invalid maxValueLength");
        return false;
    }
    proxyData.maxValueLength_ = proxyConfig.maxValueLength_;
    // maxValueLength is int32_t enum but always positive, safe to cast to size_t for comparison
    size_t maxLength = static_cast<size_t>(proxyConfig.maxValueLength_);

    if (proxyData.value_.index() == DataProxyValueType::VALUE_STRING) {
        std::string valueStr = std::get<std::string>(proxyData.value_);
        if (valueStr.size() > maxLength) {
            ZLOGE("value of proxyData %{public}s is over limit, size %{public}zu",
                URIUtils::Anonymous(proxyData.uri_).c_str(), valueStr.size());
            return false;
        }
    }

    if (proxyData.uri_.size() > URI_MAX_SIZE) {
        ZLOGE("uri of proxyData %{public}s is over limit, size %{public}zu",
            URIUtils::Anonymous(proxyData.uri_).c_str(), proxyData.uri_.size());
        return false;
    }

    CheckAndCorrectAppIdentifierList(proxyData.allowList_, "allowList", proxyData.uri_);
    CheckAndCorrectAppIdentifierList(proxyData.trustProviders_, "trustProviders", proxyData.uri_);

    return true;
}

int32_t PublishedProxyData::PutIntoTable(std::shared_ptr<KvDBDelegate> kvDelegate, int32_t user,
    uint32_t tokenId, const std::vector<std::string> &proxyDataList, const DataShareProxyData &proxyData)
{
    if (kvDelegate == nullptr) {
        ZLOGE("kv delegate is null!");
        return INNER_ERROR;
    }

    SerialDataShareProxyData serialProxyData(proxyData.uri_, proxyData.value_, proxyData.allowList_);
    serialProxyData.maxValueLength = proxyData.maxValueLength_;
    serialProxyData.isMultiValues = proxyData.isMultiValues_;
    if (proxyData.isMultiValues_) {
        serialProxyData.multiValues = proxyData.multiValues_;
    }
    serialProxyData.trustProviders = proxyData.trustProviders_;

    auto ret = kvDelegate->Upsert(KvDBDelegate::PROXYDATA_TABLE, PublishedProxyData(ProxyDataNode(
        serialProxyData, user, tokenId)));
    if (ret.first != E_OK) {
        ZLOGE("db Upsert failed, %{public}s %{public}d",
            URIUtils::Anonymous(proxyData.uri_).c_str(), ret.first);
        return INNER_ERROR;
    }

    if (!proxyDataList.empty()) {
        auto value = ProxyDataList(ProxyDataListNode(
            proxyDataList, user, tokenId));
        ret = kvDelegate->Upsert(KvDBDelegate::PROXYDATA_TABLE, value);
        if (ret.first != E_OK) {
            ZLOGE("db Upsert failed, %{public}x %{public}d", tokenId, user);
            return INNER_ERROR;
        }
    }
    return SUCCESS;
}

int32_t PublishedProxyData::InsertProxyData(std::shared_ptr<KvDBDelegate> kvDelegate, const std::string &bundleName,
    const int32_t &user, const uint32_t &tokenId, const DataShareProxyData &proxyData)
{
    if (kvDelegate == nullptr) {
        ZLOGE("kv delegate is null!");
        return INNER_ERROR;
    }

    ProxyDataListNode proxyDataList;
    std::string listFilter = Id(std::to_string(tokenId), user);
    std::string listQueryResult;
    kvDelegate->Get(KvDBDelegate::PROXYDATA_TABLE, listFilter, "{}", listQueryResult);
    if (!listQueryResult.empty()) {
        if (!ProxyDataListNode::Unmarshall(listQueryResult, proxyDataList)) {
            ZLOGE("ProxyDataListNode unmarshall failed, %{public}s",
                StringUtils::GeneralAnonymous(listQueryResult).c_str());
            return INNER_ERROR;
        }
    }
    if (proxyDataList.uris.size() >= PROXY_DATA_MAX_COUNT) {
        return OVER_LIMIT;
    }

    std::string modifyBundle;
    URIUtils::GetBundleNameFromProxyURI(proxyData.uri_, modifyBundle);
    if (modifyBundle != bundleName) {
        ZLOGE("only allowed to publish the proxyData of self bundle %{public}s, dest bundle %{public}s",
            bundleName.c_str(), modifyBundle.c_str());
        return NO_PERMISSION;
    }

    auto it = std::find(proxyDataList.uris.begin(), proxyDataList.uris.end(), proxyData.uri_);
    if (it == proxyDataList.uris.end()) {
        proxyDataList.uris.emplace_back(proxyData.uri_);
    }
    return PutIntoTable(kvDelegate, user, tokenId, proxyDataList.uris, proxyData);
}

int32_t PublishedProxyData::UpdateProxyData(std::shared_ptr<KvDBDelegate> kvDelegate, const std::string &bundleName,
    const int32_t &user, const uint32_t &tokenId, const DataShareProxyData &proxyData)
{
    if (kvDelegate == nullptr) {
        ZLOGE("kv delegate is null!");
        return INNER_ERROR;
    }
    std::vector<std::string> proxyDataList;
    return PutIntoTable(kvDelegate, user, tokenId, proxyDataList, proxyData);
}

int32_t PublishedProxyData::Query(const std::string &uri, const BundleInfo &callerBundleInfo,
    DataShareProxyData &proxyData, QueryMode mode)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return INNER_ERROR;
    }
    std::string filter = Id(uri, callerBundleInfo.userId);
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);
    if (queryResult.empty()) {
        return URI_NOT_EXIST;
    }
    ProxyDataNode data;
    if (!ProxyDataNode::Unmarshall(queryResult, data)) {
        ZLOGE("Unmarshall failed, %{public}s", StringUtils::GeneralAnonymous(queryResult).c_str());
        return INNER_ERROR;
    }
    // Type compatibility check based on query mode
    if (mode == QUERY_VALUE && data.proxyData.isMultiValues) {
        ZLOGE("QUERY_VALUE expects non-MultiValues, but got MultiValues, uri=%{public}s",
            URIUtils::Anonymous(uri).c_str());
        return INCOMPATIBLE_CONFIG_TYPE;
    }
    if (mode == QUERY_MULTIVALUES && !data.proxyData.isMultiValues) {
        ZLOGE("QUERY_MULTIVALUES expects MultiValues, but got non-MultiValues, uri=%{public}s",
            URIUtils::Anonymous(uri).c_str());
        return INCOMPATIBLE_CONFIG_TYPE;
    }
    // Field assignment
    proxyData.uri_ = data.proxyData.uri;
    proxyData.value_ = data.proxyData.value;
    proxyData.isMultiValues_ = data.proxyData.isMultiValues;
    // Return all multiValues under the URI (not filtered by caller's appIdentifier)
    if (mode == QUERY_MULTIVALUES) {
        proxyData.multiValues_ = data.proxyData.multiValues;
    }
    // Permission visibility: publisher can see all config
    if (callerBundleInfo.tokenId == data.tokenId) {
        proxyData.allowList_ = data.proxyData.allowList;
        proxyData.trustProviders_ = data.proxyData.trustProviders;
    }
    if (!VerifyPermission(callerBundleInfo, data)) {
        return NO_PERMISSION;
    }
    return SUCCESS;
}

int32_t PublishedProxyData::Upsert(const DataShareProxyData &proxyData, const BundleInfo &callerBundleInfo,
    DataShareObserver::ChangeType &type, const DataProxyConfig &proxyConfig,
    const ProxyDataUpsertMode mode)
{
    type = DataShareObserver::ChangeType::INVAILD;
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return INNER_ERROR;
    }
    auto data = proxyData;
    if (!CheckAndCorrectProxyData(data, proxyConfig, mode)) {
        return OVER_LIMIT;
    }
    std::string filter = Id(data.uri_, callerBundleInfo.userId);
    std::string queryResult;
    std::lock_guard<std::mutex> lock(mutex_);
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);
    if (queryResult.empty()) {
        return UpsertInsert(delegate, data, callerBundleInfo, type, mode);
    }
    if (mode == PUBLISH_MULTIVALUES) {
        ZLOGE("Publish an existing MultiValues is not allowed, please delete corresponding uri and then publish.");
        return INCOMPATIBLE_CONFIG_TYPE;
    }
    ProxyDataNode oldData;
    if (!ProxyDataNode::Unmarshall(queryResult, oldData)) {
        ZLOGE("ProxyDataNode unmarshall failed, %{public}s", StringUtils::GeneralAnonymous(queryResult).c_str());
        return INNER_ERROR;
    }
    if (!IsConfigCompatible(mode, oldData.proxyData.isMultiValues)) {
        ZLOGE("Type conflict: mode=%{public}d, existingIsMultiValues=%{public}d",
            mode, oldData.proxyData.isMultiValues);
        return INCOMPATIBLE_CONFIG_TYPE;
    }
    UpsertContext ctx { delegate, oldData, callerBundleInfo };
    return UpsertUpdate(ctx, data, mode, type);
}

int32_t PublishedProxyData::UpsertInsert(std::shared_ptr<KvDBDelegate> delegate,
    const DataShareProxyData &data, const BundleInfo &callerBundleInfo,
    DataShareObserver::ChangeType &type, const ProxyDataUpsertMode mode)
{
    if (mode == NORMAL_PUBLISH) {
        type = DataShareObserver::ChangeType::INSERT;
        return InsertProxyData(delegate, callerBundleInfo.bundleName,
            callerBundleInfo.userId, callerBundleInfo.tokenId, data);
    } else if (mode == PUBLISH_MULTIVALUES) {
        DataShareProxyData initData = data;
        int32_t ret = BuildInitMultiValues(initData, callerBundleInfo.appIdentifier);
        if (ret != SUCCESS) {
            return ret;
        }
        type = DataShareObserver::ChangeType::INSERT;
        return InsertProxyData(delegate, callerBundleInfo.bundleName,
            callerBundleInfo.userId, callerBundleInfo.tokenId, initData);
    }
    ZLOGE("Uri not exist when PutValue");
    return URI_NOT_EXIST;
}

int32_t PublishedProxyData::UpsertUpdate(UpsertContext &ctx, const DataShareProxyData &data,
    const ProxyDataUpsertMode mode, DataShareObserver::ChangeType &type)
{
    if (mode == NORMAL_PUBLISH) {
        if (ctx.callerBundleInfo.tokenId != ctx.oldData.tokenId) {
            ZLOGE("only allow to modify the proxyData of self bundle %{public}d, dest bundle %{public}d",
                ctx.callerBundleInfo.tokenId, ctx.oldData.tokenId);
            return NO_PERMISSION;
        }
        DataShareProxyData updateData = data;
        if (data.isValueUndefined) {
            updateData.value_ = ctx.oldData.proxyData.value;
        }
        if (data.isAllowListUndefined) {
            updateData.allowList_ = ctx.oldData.proxyData.allowList;
        }
        if (ctx.oldData.proxyData.value != updateData.value_) {
            type = DataShareObserver::ChangeType::UPDATE;
        }
        return UpdateProxyData(ctx.delegate, ctx.callerBundleInfo.bundleName,
            ctx.callerBundleInfo.userId, ctx.callerBundleInfo.tokenId, updateData);
    }
    std::string targetKey;
    DataProxyValue targetValue;
    bool found = false;
    for (const auto &[appIdentifier, keyMap] : data.multiValues_) {
        if (!keyMap.empty()) {
            targetKey = keyMap.begin()->first;
            targetValue = keyMap.begin()->second;
            found = true;
            break;
        }
    }
    if (!found) {
        ZLOGE("No valid key found in multiValues for mode %{public}d", mode);
        return KEY_NOT_EXIST;
    }
    if (mode == PUT_MULTIVALUES) {
        return UpsertPutValue(ctx, targetKey, targetValue, type);
    }
    if (mode == REMOVE_MULTIVALUES) {
        return UpsertRemoveValue(ctx, targetKey, type);
    }
    return INNER_ERROR;
}

int32_t PublishedProxyData::UpsertRemoveValue(UpsertContext &ctx, const std::string &key,
    DataShareObserver::ChangeType &type)
{
    if (!CanModifyMultiValue(ctx.oldData, key, ctx.callerBundleInfo)) {
        return KEY_NOT_EXIST;
    }
    DataShareProxyData deleteData = BuildMultiValuesAfterRemove(ctx.oldData, key, ctx.callerBundleInfo);
    type = DataShareObserver::ChangeType::UPDATE;
    return UpdateProxyData(ctx.delegate, ctx.callerBundleInfo.bundleName,
        ctx.callerBundleInfo.userId, ctx.callerBundleInfo.tokenId, deleteData);
}

size_t PublishedProxyData::CalculateCurrentTotal(const ProxyDataNode &oldData)
{
    size_t currentTotal = 0;
    for (const auto &[appIdentifier, keyMap] : oldData.proxyData.multiValues) {
        for (const auto &[key, value] : keyMap) {
            currentTotal += GetValueLength(value);
        }
    }
    return currentTotal;
}

int32_t PublishedProxyData::CheckValueAndTotalLimits(const DataProxyValue &value,
    size_t currentTotal, size_t maxValueLength)
{
    size_t valueLength = GetValueLength(value);
    if (valueLength > MAX_SINGLE_MULTIVALUE_LENGTH) {
        ZLOGE("value over limit %{public}zu > %{public}u",
            valueLength, MAX_SINGLE_MULTIVALUE_LENGTH);
        return OVER_LIMIT;
    }
    // Avoid integer overflow: check if addition would exceed maxValueLength
    if (currentTotal > maxValueLength || valueLength > maxValueLength - currentTotal) {
        ZLOGE("total over limit %{public}zu + %{public}zu > %{public}zu",
            currentTotal, valueLength, maxValueLength);
        return OVER_LIMIT;
    }
    return SUCCESS;
}

int32_t PublishedProxyData::UpsertMultiValueData(const UpsertContext &ctx, const std::string &key,
    const DataProxyValue &value, DataShareObserver::ChangeType &type)
{
    DataShareProxyData updateData = BuildMultiValuesAfterPut(ctx.oldData, key, value, ctx.callerBundleInfo);
    type = DataShareObserver::ChangeType::UPDATE;
    return UpdateProxyData(ctx.delegate, ctx.callerBundleInfo.bundleName,
        ctx.callerBundleInfo.userId, ctx.callerBundleInfo.tokenId, updateData);
}

int32_t PublishedProxyData::UpsertPutValue(UpsertContext &ctx, const std::string &key,
    const DataProxyValue &value, DataShareObserver::ChangeType &type)
{
    auto appIter = ctx.oldData.proxyData.multiValues.find(ctx.callerBundleInfo.appIdentifier);
    if (appIter == ctx.oldData.proxyData.multiValues.end()) {
        // Case 1: appIdentifier not exists - need to check permission before inserting
        if (!CanInsertMultiValues(ctx.callerBundleInfo, ctx.oldData)) {
            ZLOGE("No permission to modify MultiValues");
            return NO_PERMISSION;
        }
        size_t currentTotal = CalculateCurrentTotal(ctx.oldData);
        int32_t ret = CheckValueAndTotalLimits(value, currentTotal, ctx.oldData.proxyData.maxValueLength);
        if (ret != SUCCESS) {
            return ret;
        }
        return UpsertMultiValueData(ctx, key, value, type);
    }

    auto keyIter = appIter->second.find(key);
    if (keyIter != appIter->second.end()) {
        // Case 2a: key already exists under caller.appIdentifier, update directly without permission check
        size_t oldSize = GetValueLength(keyIter->second);
        size_t newSize = GetValueLength(value);

        size_t currentTotal = CalculateCurrentTotal(ctx.oldData);
        // maxValueLength is int32_t enum but always positive (4K/100K), safe to cast to size_t
        size_t maxValueLengthSize = static_cast<size_t>(ctx.oldData.proxyData.maxValueLength);

        // Avoid integer overflow: use subtraction-based check
        if (newSize > oldSize) {
            size_t sizeDiff = newSize - oldSize;
            if (currentTotal > maxValueLengthSize || sizeDiff > maxValueLengthSize - currentTotal) {
                ZLOGE("total over limit after update %{public}zu + %{public}zu > %{public}zu",
                    currentTotal, sizeDiff, maxValueLengthSize);
                return OVER_LIMIT;
            }
        }
        return UpsertMultiValueData(ctx, key, value, type);
    }

    // Case 2b: key not exists - insert to existing appIdentifier
    size_t callerAppendCount = appIter->second.size();
    if (callerAppendCount >= MAX_MULTIVALUE_COUNT_PER_APP) {
        ZLOGE("app %{public}s append count over limit %{public}zu >= %{public}zu",
            ctx.callerBundleInfo.appIdentifier.c_str(), callerAppendCount, MAX_MULTIVALUE_COUNT_PER_APP);
        return OVER_LIMIT;
    }
    size_t currentTotal = CalculateCurrentTotal(ctx.oldData);
    int32_t ret = CheckValueAndTotalLimits(value, currentTotal, ctx.oldData.proxyData.maxValueLength);
    if (ret != SUCCESS) {
        return ret;
    }
    return UpsertMultiValueData(ctx, key, value, type);
}

int32_t PublishedProxyData::UpdateProxyDataList(std::shared_ptr<KvDBDelegate> delegate, const std::string &uri,
    const BundleInfo &callerBundleInfo)
{
    if (delegate == nullptr) {
        ZLOGE("delegate is null");
        return INNER_ERROR;
    }
    std::vector<std::string> uris;
    if (ProxyDataList::Query(callerBundleInfo.tokenId, callerBundleInfo.userId, uris) <= 0) {
        ZLOGI("get bundle %{public}s's proxyData failed", callerBundleInfo.bundleName.c_str());
        return INNER_ERROR;
    }

    auto it = std::find(uris.begin(), uris.end(), uri);
    if (it != uris.end()) {
        uris.erase(it);
    }

    if (uris.empty()) {
        std::string filter = Id(std::to_string(callerBundleInfo.tokenId), callerBundleInfo.userId);
        auto ret = delegate->Delete(KvDBDelegate::PROXYDATA_TABLE, filter);
        if (ret.first != E_OK) {
            ZLOGE("db Delete failed, %{public}x %{public}d", callerBundleInfo.tokenId, ret.first);
            return INNER_ERROR;
        }
    } else {
        auto ret = delegate->Upsert(KvDBDelegate::PROXYDATA_TABLE, ProxyDataList(ProxyDataListNode(
            uris, callerBundleInfo.userId, callerBundleInfo.tokenId)));
        if (ret.first != E_OK) {
            ZLOGE("db Upsert failed, %{public}x %{public}d", callerBundleInfo.tokenId, ret.first);
            return INNER_ERROR;
        }
    }
    return E_OK;
}

int32_t PublishedProxyData::Delete(const std::string &uri, const BundleInfo &callerBundleInfo,
    DataShareProxyData &oldProxyData, DataShareObserver::ChangeType &type)
{
    type = DataShareObserver::ChangeType::INVAILD;
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return INNER_ERROR;
    }

    std::string bundleName;
    URIUtils::GetBundleNameFromProxyURI(uri, bundleName);
    if (callerBundleInfo.bundleName != bundleName) {
        return NO_PERMISSION;
    }
    std::string queryResult;
    std::lock_guard<std::mutex> lock(mutex_);
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, Id(uri, callerBundleInfo.userId), "{}", queryResult);
    if (queryResult.empty()) {
        return URI_NOT_EXIST;
    }

    ProxyDataNode oldData;
    if (!ProxyDataNode::Unmarshall(queryResult, oldData)) {
        ZLOGE("Unmarshall failed, %{public}s", StringUtils::GeneralAnonymous(queryResult).c_str());
        return INNER_ERROR;
    }
    if (callerBundleInfo.tokenId != oldData.tokenId) {
        return NO_PERMISSION;
    }
    auto ret = delegate->Delete(KvDBDelegate::PROXYDATA_TABLE, Id(uri, callerBundleInfo.userId));
    if (ret.first != E_OK) {
        ZLOGE("db Delete failed, %{public}s %{public}d", URIUtils::Anonymous(uri).c_str(), ret.first);
        return INNER_ERROR;
    }

    int32_t errCode = UpdateProxyDataList(delegate, uri, callerBundleInfo);
    if (errCode != E_OK) {
        return errCode;
    }

    oldProxyData = DataShareProxyData(oldData.proxyData.uri, oldData.proxyData.value, oldData.proxyData.allowList);
    type = DataShareObserver::ChangeType::DELETE;
    return SUCCESS;
}

int32_t ProxyDataList::Query(uint32_t tokenId, int32_t userId, std::vector<std::string> &proxyDataList)
{
    int32_t invalidNum = -1;
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return invalidNum;
    }
    std::string filter = Id(std::to_string(tokenId), userId);
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);
    if (queryResult.empty()) {
        return invalidNum;
    }
    ProxyDataListNode data;
    if (!ProxyDataListNode::Unmarshall(queryResult, data)) {
        ZLOGE("Unmarshall failed, %{public}s", StringUtils::GeneralAnonymous(queryResult).c_str());
        return invalidNum;
    }

    proxyDataList = std::move(data.uris);
    return proxyDataList.size();
}

bool ProxyDataManager::GetCrossAppSharedConfig(const std::string &bundleName, int32_t user,
    int32_t index, std::vector<DataShareProxyData> &proxyDatas)
{
    BundleConfig bundleConfig;
    auto ret = BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(bundleName, user, bundleConfig, index);
    if (ret != E_OK) {
        ZLOGE("Get bundleInfo falied, ret : %{public}d", ret);
        return false;
    }
    std::vector<std::string> resourcePath;
    std::vector<SerialDataShareProxyData> datas;
    std::for_each(bundleConfig.hapModuleInfos.begin(),
        bundleConfig.hapModuleInfos.end(), [&datas](const HapModuleInfo hapModuleInfo) {
        if (!hapModuleInfo.crossAppSharedConfig.resourcePath.empty()) {
            auto [ret, data] = DataShareProfileConfig::GetCrossAppSharedConfig(
                hapModuleInfo.crossAppSharedConfig.resourcePath, hapModuleInfo.resourcePath, hapModuleInfo.hapPath);
            if (ret == SUCCESS) {
                datas.insert(datas.end(), data.begin(), data.end());
            } else {
                ZLOGE("get shareConfig failed, err: %{public}d", ret);
            }
        }
    });
    std::for_each(datas.begin(), datas.end(), [&proxyDatas](auto &data) {
        proxyDatas.emplace_back(std::move(data.uri), std::move(data.value), std::move(data.allowList));
    });
    return true;
}

void ProxyDataManager::OnAppInstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    std::vector<DataShareProxyData> proxyDatas;
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = bundleName;
    callerBundleInfo.userId = user;
    callerBundleInfo.appIndex = index;
    callerBundleInfo.tokenId = tokenId;
    if (!GetCrossAppSharedConfig(bundleName, user, index, proxyDatas)) {
        ZLOGE("GetCrossAppSharedConfig after install failed");
        return;
    }
    DataShareObserver::ChangeType type;
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_100K;
    std::for_each(proxyDatas.begin(), proxyDatas.end(),
        [user, bundleName, callerBundleInfo, &type, config](const DataShareProxyData proxyData) {
        PublishedProxyData::Upsert(proxyData, callerBundleInfo, type, config);
    });
}

void ProxyDataManager::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    std::vector<DataShareProxyData> proxyDatas;
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = bundleName;
    callerBundleInfo.userId = user;
    callerBundleInfo.appIndex = index;
    callerBundleInfo.tokenId = tokenId;
    DataShareObserver::ChangeType type;
    if (!GetCrossAppSharedConfig(bundleName, user, index, proxyDatas)) {
        ZLOGE("GetCrossAppSharedConfig after install failed");
        return;
    }
    std::vector<std::string> uris;
    auto count = ProxyDataList::Query(tokenId, user, uris);
    if (count <= 0) {
        ZLOGI("bundle %{public}s has no proxyData", bundleName.c_str());
    }
    DataShareProxyData oldProxyData;
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_100K;
    for (const auto &uri : uris) {
        PublishedProxyData::Delete(uri, callerBundleInfo, oldProxyData, type);
    }
    std::for_each(proxyDatas.begin(), proxyDatas.end(),
        [user, bundleName, callerBundleInfo, &type, config](const DataShareProxyData proxyData) {
        PublishedProxyData::Upsert(proxyData, callerBundleInfo, type, config);
    });
}

void ProxyDataManager::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    DataShareProxyData proxyData;
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = bundleName;
    callerBundleInfo.userId = user;
    callerBundleInfo.appIndex = index;
    callerBundleInfo.tokenId = tokenId;
    DataShareObserver::ChangeType type;
    std::vector<std::string> uris;
    auto count = ProxyDataList::Query(tokenId, user, uris);
    if (count <= 0) {
        ZLOGI("bundle %{public}s has no proxyData", bundleName.c_str());
    }
    for (const auto &uri : uris) {
        PublishedProxyData::Delete(uri, callerBundleInfo, proxyData, type);
    }
}

bool PublishedProxyData::IsConfigCompatible(const ProxyDataUpsertMode mode, bool existingIsMultiValues)
{
    if (mode == NORMAL_PUBLISH && existingIsMultiValues) {
        return false;
    }
    if ((mode == PUT_MULTIVALUES || mode == REMOVE_MULTIVALUES) && !existingIsMultiValues) {
        return false;
    }
    return true;
}

bool PublishedProxyData::CanInsertMultiValues(const BundleInfo &callerBundleInfo, const ProxyDataNode &data)
{
    if (callerBundleInfo.tokenId == data.tokenId) {
        return true;
    }
    for (const auto &item : data.proxyData.trustProviders) {
        if (item == ALLOW_ALL || callerBundleInfo.appIdentifier == item) {
            return true;
        }
    }
    return false;
}

bool PublishedProxyData::CanModifyMultiValue(const ProxyDataNode &data, const std::string &key,
    const BundleInfo &callerBundleInfo)
{
    for (const auto &[appIdentifier, keyMap] : data.proxyData.multiValues) {
        auto keyIter = keyMap.find(key);
        if (keyIter != keyMap.end()) {
            if (callerBundleInfo.tokenId == data.tokenId) {
                return true;
            }
            if (callerBundleInfo.appIdentifier == appIdentifier) {
                return true;
            }
            return false;
        }
    }
    return false;
}

int32_t PublishedProxyData::BuildInitMultiValues(DataShareProxyData &data, const std::string &appIdentifier)
{
    if (!data.multiValues_.empty()) {
        std::map<std::string, DataProxyValue> mergedData;
        size_t totalSize = 0;
        for (auto &[placeholderId, keyMap] : data.multiValues_) {
            for (auto &[key, value] : keyMap) {
                mergedData[key] = value;
                totalSize += GetValueLength(value);
            }
        }

        // maxValueLength is int32_t enum but always positive, safe to cast to size_t for comparison
        if (totalSize > static_cast<size_t>(data.maxValueLength_)) {
            ZLOGE("BuildInitMultiValues total size over limit, size %{public}zu > max %{public}zu",
                totalSize, static_cast<size_t>(data.maxValueLength_));
            return OVER_LIMIT;
        }

        data.multiValues_.clear();
        data.multiValues_[appIdentifier] = mergedData;
    }
    data.value_ = "";
    data.isMultiValues_ = true;
    return SUCCESS;
}

DataShareProxyData PublishedProxyData::BuildMultiValuesAfterRemove(const ProxyDataNode &existing,
    const std::string &key, const BundleInfo &callerBundleInfo)
{
    DataShareProxyData result;
    result.uri_ = existing.proxyData.uri;
    result.value_ = "";
    result.allowList_ = existing.proxyData.allowList;
    result.trustProviders_ = existing.proxyData.trustProviders;
    result.isMultiValues_ = true;

    result.multiValues_ = existing.proxyData.multiValues;
    auto appIter = result.multiValues_.find(callerBundleInfo.appIdentifier);
    if (appIter != result.multiValues_.end()) {
        appIter->second.erase(key);
    }

    result.maxValueLength_ = existing.proxyData.maxValueLength;
    return result;
}

DataShareProxyData PublishedProxyData::BuildMultiValuesAfterPut(const ProxyDataNode &existing,
    const std::string &key, const DataProxyValue &value, const BundleInfo &callerBundleInfo)
{
    DataShareProxyData result;
    result.uri_ = existing.proxyData.uri;
    result.value_ = "";
    result.allowList_ = existing.proxyData.allowList;
    result.trustProviders_ = existing.proxyData.trustProviders;
    result.isMultiValues_ = true;

    result.multiValues_ = existing.proxyData.multiValues;
    result.multiValues_[callerBundleInfo.appIdentifier][key] = value;

    result.maxValueLength_ = existing.proxyData.maxValueLength;
    return result;
}
} // namespace OHOS::DataShare