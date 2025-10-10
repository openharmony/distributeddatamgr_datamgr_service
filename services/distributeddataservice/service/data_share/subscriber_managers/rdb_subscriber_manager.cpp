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
#define LOG_TAG "RdbSubscriberManager"

#include "rdb_subscriber_manager.h"

#include <cinttypes>
#include <utility>

#include "ipc_skeleton.h"
#include "general/load_config_data_info_strategy.h"
#include "log_print.h"
#include "scheduler_manager.h"
#include "template_data.h"
#include "utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
bool TemplateManager::Get(const Key &key, int32_t userId, Template &tpl)
{
    return TemplateData::Query(Id(TemplateData::GenId(key.uri, key.bundleName, key.subscriberId), userId), tpl) == E_OK;
}

int32_t TemplateManager::Add(const Key &key, int32_t userId, const Template &tpl)
{
    auto status = TemplateData::Add(key.uri, userId, key.bundleName, key.subscriberId, tpl);
    if (!status) {
        ZLOGE("Add failed, %{public}d", status);
        return E_ERROR;
    }
    return E_OK;
}

int32_t TemplateManager::Delete(const Key &key, int32_t userId)
{
    auto status = TemplateData::Delete(key.uri, userId, key.bundleName, key.subscriberId);
    if (!status) {
        ZLOGE("Delete failed, %{public}d", status);
        return E_ERROR;
    }
    SchedulerManager::GetInstance().Stop(key);
    return E_OK;
}

Key::Key(const std::string &uri, int64_t subscriberId, const std::string &bundleName)
    : uri(uri), subscriberId(subscriberId), bundleName(bundleName)
{
}

bool Key::operator==(const Key &rhs) const
{
    return uri == rhs.uri && subscriberId == rhs.subscriberId && bundleName == rhs.bundleName;
}

bool Key::operator!=(const Key &rhs) const
{
    return !(rhs == *this);
}
bool Key::operator<(const Key &rhs) const
{
    if (uri < rhs.uri) {
        return true;
    }
    if (rhs.uri < uri) {
        return false;
    }
    if (subscriberId < rhs.subscriberId) {
        return true;
    }
    if (rhs.subscriberId < subscriberId) {
        return false;
    }
    return bundleName < rhs.bundleName;
}
bool Key::operator>(const Key &rhs) const
{
    return rhs < *this;
}
bool Key::operator<=(const Key &rhs) const
{
    return !(rhs < *this);
}
bool Key::operator>=(const Key &rhs) const
{
    return !(*this < rhs);
}

TemplateManager::TemplateManager() {}

TemplateManager &TemplateManager::GetInstance()
{
    static TemplateManager manager;
    return manager;
}

RdbSubscriberManager &RdbSubscriberManager::GetInstance()
{
    static RdbSubscriberManager manager;
    return manager;
}

int RdbSubscriberManager::Add(const Key &key, const sptr<IDataProxyRdbObserver> observer,
    std::shared_ptr<Context> context, std::shared_ptr<ExecutorPool> executorPool)
{
    int result = E_OK;
    rdbCache_.Compute(key, [&observer, &context, executorPool, this](const auto &key, auto &value) {
        ZLOGI("add subscriber, uri %{public}s tokenId 0x%{public}x",
            URIUtils::Anonymous(key.uri).c_str(), context->callerTokenId);
        auto callerTokenId = IPCSkeleton::GetCallingTokenID();
        auto callerPid = IPCSkeleton::GetCallingPid();
        value.emplace_back(observer, context->callerTokenId, callerTokenId, callerPid, context->visitedUserId);
        std::vector<ObserverNode> node;
        node.emplace_back(observer, context->callerTokenId, callerTokenId, callerPid, context->visitedUserId);
        bool isFirstSubscribe = SchedulerManager::GetInstance().Add(key);
        ExecutorPool::Task task = [key, node, context, isFirstSubscribe, this]() {
            LoadConfigDataInfoStrategy loadDataInfo;
            if (!loadDataInfo(context)) {
                ZLOGE("loadDataInfo failed, uri %{public}s tokenId 0x%{public}x",
                    URIUtils::Anonymous(key.uri).c_str(), context->callerTokenId);
                return;
            }
            DistributedData::StoreMetaData metaData = RdbSubscriberManager::GenMetaDataFromContext(context);
            Notify(key, context->visitedUserId, node, metaData);
            if (isFirstSubscribe) {
                SchedulerManager::GetInstance().Execute(key, context->visitedUserId, metaData);
            }
        };
        executorPool->Execute(task);
        return true;
    });
    return result;
}

int RdbSubscriberManager::Delete(const Key &key, uint32_t firstCallerTokenId)
{
    auto result =
        rdbCache_.ComputeIfPresent(key, [&firstCallerTokenId, this](const auto &key,
            std::vector<ObserverNode> &value) {
            ZLOGI("delete subscriber, uri %{public}s tokenId 0x%{public}x",
                URIUtils::Anonymous(key.uri).c_str(), firstCallerTokenId);
            for (auto it = value.begin(); it != value.end();) {
                if (it->firstCallerTokenId == firstCallerTokenId) {
                    ZLOGI("erase start");
                    it = value.erase(it);
                } else {
                    it++;
                }
            }
            if (value.empty()) {
                SchedulerManager::GetInstance().Stop(key);
            }
            return !value.empty();
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

void RdbSubscriberManager::Delete(uint32_t callerTokenId, uint32_t callerPid)
{
    rdbCache_.EraseIf([&callerTokenId, &callerPid, this](const auto &key, std::vector<ObserverNode> &value) {
        for (auto it = value.begin(); it != value.end();) {
            if (it->callerTokenId == callerTokenId && it->callerPid == callerPid) {
                it = value.erase(it);
            } else {
                it++;
            }
        }
        if (value.empty()) {
            ZLOGI("delete timer, subId %{public}" PRId64 ", bundleName %{public}s, tokenId %{public}x, uri %{public}s.",
                key.subscriberId, key.bundleName.c_str(), callerTokenId,
                URIUtils::Anonymous(key.uri).c_str());
            SchedulerManager::GetInstance().Stop(key);
        }
        return value.empty();
    });
}

int RdbSubscriberManager::Disable(const Key &key, uint32_t firstCallerTokenId)
{
    bool isAllDisabled = true;
    bool result =
        rdbCache_.ComputeIfPresent(key, [&firstCallerTokenId, &isAllDisabled, this](const auto &key,
            std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end(); it++) {
                if (it->firstCallerTokenId == firstCallerTokenId) {
                    it->enabled = false;
                    it->isNotifyOnEnabled = false;
                }
                if (it->enabled) {
                    isAllDisabled = false;
                }
            }
            return true;
        });
    if (isAllDisabled) {
        SchedulerManager::GetInstance().Disable(key);
    }
    if (!result) {
        ZLOGE("disable failed, uri is %{public}s, bundleName is %{public}s, subscriberId is %{public}" PRId64,
            URIUtils::Anonymous(key.uri).c_str(), key.bundleName.c_str(), key.subscriberId);
    }
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

int RdbSubscriberManager::Enable(const Key &key, std::shared_ptr<Context> context)
{
    bool isChanged = false;
    DistributedData::StoreMetaData metaData;
    bool result = rdbCache_.ComputeIfPresent(key, [&context, &metaData, &isChanged, this](const auto &key,
        std::vector<ObserverNode> &value) {
        for (auto it = value.begin(); it != value.end(); it++) {
            if (it->firstCallerTokenId != context->callerTokenId) {
                continue;
            }
            it->enabled = true;
            LoadConfigDataInfoStrategy loadDataInfo;
            if (!loadDataInfo(context)) {
                return true;
            }
            isChanged = true;
            metaData = RdbSubscriberManager::GenMetaDataFromContext(context);
            if (it->isNotifyOnEnabled) {
                std::vector<ObserverNode> node;
                node.emplace_back(it->observer, context->callerTokenId);
                Notify(key, context->visitedUserId, node, metaData);
            }
        }
        return true;
    });
    if (isChanged) {
        SchedulerManager::GetInstance().Enable(key, context->visitedUserId, metaData);
    }
    if (!result) {
        ZLOGE("enable failed, uri is %{public}s, bundleName is %{public}s, subscriberId is %{public}" PRId64,
            URIUtils::Anonymous(key.uri).c_str(), key.bundleName.c_str(), key.subscriberId);
    }
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

void RdbSubscriberManager::Emit(const std::string &uri, std::shared_ptr<Context> context)
{
    if (!URIUtils::IsDataProxyURI(uri)) {
        return;
    }
    if (context->calledSourceDir.empty()) {
        LoadConfigDataInfoStrategy loadDataInfo;
        loadDataInfo(context);
    }
    DistributedData::StoreMetaData metaData = RdbSubscriberManager::GenMetaDataFromContext(context);
    rdbCache_.ForEach([&uri, &context, &metaData, this](const Key &key, std::vector<ObserverNode> &val) {
        if (key.uri != uri) {
            return false;
        }
        Notify(key, context->visitedUserId, val, metaData);
        SetObserverNotifyOnEnabled(val);
        return false;
    });
    SchedulerManager::GetInstance().Execute(
        uri, context->visitedUserId, metaData);
}

void RdbSubscriberManager::Emit(const std::string &uri, int32_t userId,
    DistributedData::StoreMetaData &metaData)
{
    if (!URIUtils::IsDataProxyURI(uri)) {
        return;
    }
    bool hasObserver = false;
    rdbCache_.ForEach([&uri, &userId, &metaData, &hasObserver, this](const Key &key, std::vector<ObserverNode> &val) {
        if (key.uri != uri) {
            return false;
        }
        hasObserver = true;
        Notify(key, userId, val, metaData);
        SetObserverNotifyOnEnabled(val);
        return false;
    });
    if (!hasObserver) {
        return;
    }
    SchedulerManager::GetInstance().Execute(
        uri, userId, metaData);
}

void RdbSubscriberManager::SetObserverNotifyOnEnabled(std::vector<ObserverNode> &nodes)
{
    for (auto &node : nodes) {
        if (!node.enabled) {
            node.isNotifyOnEnabled = true;
        }
    }
}

std::vector<Key> RdbSubscriberManager::GetKeysByUri(const std::string &uri)
{
    std::vector<Key> results;
    rdbCache_.ForEach([&uri, &results](const Key &key, std::vector<ObserverNode> &val) {
        if (key.uri != uri) {
            return false;
        }
        results.emplace_back(key);
        return false;
    });
    return results;
}

void RdbSubscriberManager::EmitByKey(const Key &key, int32_t userId, const DistributedData::StoreMetaData &metaData)
{
    if (!URIUtils::IsDataProxyURI(key.uri)) {
        return;
    }
    rdbCache_.ComputeIfPresent(key, [&userId, &metaData, this](const Key &key, auto &val) {
        Notify(key, userId, val, metaData);
        SetObserverNotifyOnEnabled(val);
        return true;
    });
}

int RdbSubscriberManager::GetEnableObserverCount(const Key &key)
{
    auto pair = rdbCache_.Find(key);
    if (!pair.first) {
        return 0;
    }
    int count = 0;
    for (const auto &observer : pair.second) {
        if (observer.enabled) {
            count++;
        }
    }
    return count;
}

int RdbSubscriberManager::Notify(const Key &key, int32_t userId, const std::vector<ObserverNode> &val,
    const DistributedData::StoreMetaData &metaData)
{
    Template tpl;
    if (!TemplateManager::GetInstance().Get(key, userId, tpl)) {
        ZLOGE("template undefined, %{public}s, %{public}" PRId64 ", %{public}s",
            URIUtils::Anonymous(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        return E_TEMPLATE_NOT_EXIST;
    }
    DistributedData::StoreMetaData meta = metaData;
    meta.user = std::to_string(userId);
    auto delegate = DBDelegate::Create(meta, key.uri);
    if (delegate == nullptr) {
        ZLOGE("Create fail %{public}s %{public}s", URIUtils::Anonymous(key.uri).c_str(),
            key.bundleName.c_str());
        return E_ERROR;
    }
    RdbChangeNode changeNode;
    changeNode.uri_ = key.uri;
    changeNode.templateId_.subscriberId_ = key.subscriberId;
    changeNode.templateId_.bundleName_ = key.bundleName;
    for (const auto &predicate : tpl.predicates_) {
        std::string result = delegate->Query(predicate.selectSql_);
        if (result.empty()) {
            continue;
        }
        changeNode.data_.emplace_back("{\"" + predicate.key_ + "\":" + result + "}");
    }
    if (!tpl.update_.empty()) {
        auto [errCode, rowCount] = delegate->UpdateSql(tpl.update_);
        if (errCode != E_OK) {
            ZLOGE("Update failed, err:%{public}d, %{public}s, %{public}" PRId64 ", %{public}s",
            errCode, URIUtils::Anonymous(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        }
    }

    ZLOGI("emit, valSize: %{public}zu, dataSize:%{public}zu, uri:%{public}s,",
        val.size(), changeNode.data_.size(), URIUtils::Anonymous(changeNode.uri_).c_str());
    for (const auto &callback : val) {
        // not notify across user
        if (callback.userId != userId && userId != 0 && callback.userId != 0) {
            ZLOGI("Not allow across notify, uri:%{public}s, from %{public}d to %{public}d.",
                URIUtils::Anonymous(changeNode.uri_).c_str(), userId, callback.userId);
            continue;
        }
        if (callback.enabled && callback.observer != nullptr) {
            callback.observer->OnChangeFromRdb(changeNode);
        }
    }
    return E_OK;
}

void RdbSubscriberManager::Clear()
{
    rdbCache_.Clear();
}

void RdbSubscriberManager::Emit(const std::string &uri, int64_t subscriberId,
    const std::string &bundleName, std::shared_ptr<Context> context)
{
    if (!URIUtils::IsDataProxyURI(uri)) {
        return;
    }
    if (context->calledSourceDir.empty()) {
        LoadConfigDataInfoStrategy loadDataInfo;
        loadDataInfo(context);
    }
    DistributedData::StoreMetaData metaData = RdbSubscriberManager::GenMetaDataFromContext(context);
    rdbCache_.ForEach([&uri, &context, &subscriberId, &metaData, this](const Key &key, std::vector<ObserverNode> &val) {
        if (key.uri != uri || key.subscriberId != subscriberId) {
            return false;
        }
        Notify(key, context->visitedUserId, val, metaData);
        SetObserverNotifyOnEnabled(val);
        return false;
    });
    Key executeKey(uri, subscriberId, bundleName);
    SchedulerManager::GetInstance().Start(executeKey, context->visitedUserId, metaData);
}

DistributedData::StoreMetaData RdbSubscriberManager::GenMetaDataFromContext(const std::shared_ptr<Context> context)
{
    DistributedData::StoreMetaData metaData;
    metaData.tokenId = context->calledTokenId;
    metaData.dataDir = context->calledSourceDir;
    metaData.storeId = context->calledStoreName;
    metaData.haMode = context->haMode;
    metaData.isEncrypt = context->isEncryptDb;
    metaData.bundleName = context->calledBundleName;
    return metaData;
}

RdbSubscriberManager::ObserverNode::ObserverNode(const sptr<IDataProxyRdbObserver> &observer,
    uint32_t firstCallerTokenId, uint32_t callerTokenId, uint32_t callerPid, int32_t userId): observer(observer),
    firstCallerTokenId(firstCallerTokenId), callerTokenId(callerTokenId), callerPid(callerPid), userId(userId)
{
}
} // namespace OHOS::DataShare