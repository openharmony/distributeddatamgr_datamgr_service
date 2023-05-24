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
#define LOG_TAG "TemplateManager"

#include "template_manager.h"

#include "general/load_config_data_info_strategy.h"

#include "db_delegate.h"
#include "log_print.h"
#include "published_data.h"
#include "scheduler_manager.h"
#include "template_data.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
bool TemplateManager::GetTemplate(
    const std::string &uri, const int64_t subscriberId, const std::string &bundleName, Template &tpl)
{
    return TemplateData::Query(Id(TemplateData::GenId(uri, bundleName, subscriberId)), tpl) == E_OK;
}

bool TemplateManager::AddTemplate(const std::string &uri, const TemplateId &tplId, const Template &tpl)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return false;
    }
    TemplateData data(uri, tplId.bundleName_, tplId.subscriberId_, tpl);
    auto status = delegate->Upsert(KvDBDelegate::TEMPLATE_TABLE, data);
    if (status != E_OK) {
        ZLOGE("db Upsert failed, %{public}d", status);
        return false;
    }
    return true;
}

bool TemplateManager::DelTemplate(const std::string &uri, const TemplateId &tplId)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return false;
    }
    auto status = delegate->DeleteById(
        KvDBDelegate::TEMPLATE_TABLE, Id(TemplateData::GenId(uri, tplId.bundleName_, tplId.subscriberId_)));
    if (status != E_OK) {
        ZLOGE("db DeleteById failed, %{public}d", status);
        return false;
    }
    SchedulerManager::GetInstance().RemoveTimer(Key(uri, tplId.subscriberId_, tplId.bundleName_));
    return true;
}

Key::Key(const std::string &uri, const int64_t subscriberId, const std::string &bundleName)
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

int RdbSubscriberManager::AddRdbSubscriber(const std::string &uri, const TemplateId &tplId,
    const sptr<IDataProxyRdbObserver> observer, std::shared_ptr<Context> context,
    std::shared_ptr<ExecutorPool> executorPool)
{
    int result = E_OK;
    Key key(uri, tplId.subscriberId_, tplId.bundleName_);
    rdbCache_.Compute(key, [&observer, &context, executorPool, this](const auto &key, std::vector<ObserverNode> &value) {
        ZLOGI("add subscriber, uri %{private}s tokenId %{public}d", key.uri.c_str(), context->callerTokenId);
        std::vector<ObserverNode> node;
        node.emplace_back(observer, context->callerTokenId);
        ExecutorPool::Task task = [key, node, context, this]() {
            LoadConfigDataInfoStrategy loadDataInfo;
            if (loadDataInfo(context)) {
                Notify(key, node, context->calledSourceDir, context->version);
            }
        };
        executorPool->Execute(task);
        value.emplace_back(observer, context->callerTokenId);
        if (GetEnableObserverCount(key) == 1) {
            SchedulerManager::GetInstance().Execute(key, context->calledSourceDir, context->version);
        }
        return true;
    });
    return result;
}

int RdbSubscriberManager::DelRdbSubscriber(
    const std::string &uri, const TemplateId &tplId, const uint32_t callerTokenId)
{
    Key key(uri, tplId.subscriberId_, tplId.bundleName_);
    auto result =
        rdbCache_.ComputeIfPresent(key, [&callerTokenId, this](const auto &key, std::vector<ObserverNode> &value) {
            ZLOGI("delete subscriber, uri %{private}s tokenId %{public}d", key.uri.c_str(), callerTokenId);
            for (auto it = value.begin(); it != value.end();) {
                if (it->callerTokenId == callerTokenId) {
                    ZLOGI("erase start");
                    it = value.erase(it);
                } else {
                    it++;
                }
            }
            if (GetEnableObserverCount(key) == 0) {
                SchedulerManager::GetInstance().RemoveTimer(key);
            }
            return !value.empty();
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

int RdbSubscriberManager::DisableRdbSubscriber(
    const std::string &uri, const TemplateId &tplId, const uint32_t callerTokenId)
{
    Key key(uri, tplId.subscriberId_, tplId.bundleName_);
    auto result =
        rdbCache_.ComputeIfPresent(key, [&callerTokenId, this](const auto &key, std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end(); it++) {
                if (it->callerTokenId == callerTokenId) {
                    it->enabled = false;
                }
            }
            if (GetEnableObserverCount(key) == 0) {
                SchedulerManager::GetInstance().RemoveTimer(key);
            }
            return true;
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

int RdbSubscriberManager::EnableRdbSubscriber(
    const std::string &uri, const TemplateId &tplId, std::shared_ptr<Context> context)
{
    Key key(uri, tplId.subscriberId_, tplId.bundleName_);
    auto result = rdbCache_.ComputeIfPresent(key, [&context, this](const auto &key, std::vector<ObserverNode> &value) {
        for (auto it = value.begin(); it != value.end(); it++) {
            if (it->callerTokenId == context->callerTokenId) {
                it->enabled = true;
                std::vector<ObserverNode> node;
                node.emplace_back(it->observer, context->callerTokenId);
                LoadConfigDataInfoStrategy loadDataInfo;
                if (loadDataInfo(context)) {
                    Notify(key, node, context->calledSourceDir, context->version);
                }
            }
            if (GetEnableObserverCount(key) == 1) {
                SchedulerManager::GetInstance().Execute(key, context->calledSourceDir, context->version);
            }
        }
        return true;
    });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

void RdbSubscriberManager::Emit(const std::string &uri, std::shared_ptr<Context> context)
{
    if (!URIUtils::IsDataProxyURI(uri)) {
        return;
    }
    rdbCache_.ForEach([&uri, &context, this](const Key &key, std::vector<ObserverNode> &val) {
        if (key.uri != uri) {
            return false;
        }
        Notify(key, val, context->calledSourceDir, context->version);
        return false;
    });
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

void RdbSubscriberManager::EmitByKey(const Key &key, const std::string &rdbPath, int version)
{
    if (!URIUtils::IsDataProxyURI(key.uri)) {
        return;
    }
    rdbCache_.ComputeIfPresent(key, [&rdbPath, &version, this](const Key &key, std::vector<ObserverNode> &val) {
        Notify(key, val, rdbPath, version);
        return true;
    });
}

int RdbSubscriberManager::GetObserverCount(const Key &key)
{
    auto pair = rdbCache_.Find(key);
    if (!pair.first) {
        return 0;
    }
    return pair.second.size();
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

int RdbSubscriberManager::Notify(
    const Key &key, const std::vector<ObserverNode> &val, const std::string &rdbDir, int rdbVersion)
{
    Template tpl;
    if (!TemplateManager::GetInstance().GetTemplate(key.uri, key.subscriberId, key.bundleName, tpl)) {
        ZLOGE("template undefined, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        return E_TEMPLATE_NOT_EXIST;
    }
    auto delegate = DBDelegate::Create(rdbDir, rdbVersion, true);
    if (delegate == nullptr) {
        ZLOGE("Create fail %{public}s %{public}s", DistributedData::Anonymous::Change(key.uri).c_str(),
            key.bundleName.c_str());
        return E_ERROR;
    }
    RdbChangeNode changeNode;
    changeNode.uri_ = key.uri;
    changeNode.templateId_.subscriberId_ = key.subscriberId;
    changeNode.templateId_.bundleName_ = key.bundleName;
    for (const auto &predicate : tpl.predicates_) {
        std::string result =  delegate->Query(predicate.selectSql_);
        changeNode.data_.emplace_back("{\"" + predicate.key_ + "\":" + result + "}");
    }

    ZLOGI("emit, size %{public}zu %{private}s", val.size(), changeNode.uri_.c_str());
    for (const auto &callback : val) {
        if (callback.enabled && callback.observer != nullptr) {
            callback.observer->OnChangeFromRdb(changeNode);
        }
    }
    return E_OK;
}

PublishedDataSubscriberManager &PublishedDataSubscriberManager::GetInstance()
{
    static PublishedDataSubscriberManager manager;
    return manager;
}

int PublishedDataSubscriberManager::AddSubscriber(const std::string &key, const std::string &callerBundleName,
    const int64_t subscriberId, const sptr<IDataProxyPublishedDataObserver> observer, const uint32_t callerTokenId)
{
    PublishedDataKey publishedDataKey(key, callerBundleName, subscriberId);
    publishedDataCache.Compute(publishedDataKey,
        [&observer, &callerTokenId, this](const PublishedDataKey &key, std::vector<ObserverNode> &value) {
            ZLOGI("add publish subscriber, uri %{private}s tokenId %{public}d", key.key.c_str(), callerTokenId);
            value.emplace_back(observer, callerTokenId);
            return true;
        });
    return E_OK;
}

int PublishedDataSubscriberManager::DelSubscriber(const std::string &uri, const std::string &callerBundleName,
    const int64_t subscriberId, const uint32_t callerTokenId)
{
    PublishedDataKey key(uri, callerBundleName, subscriberId);
    auto result =
        publishedDataCache.ComputeIfPresent(key, [&callerTokenId](const auto &key, std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end();) {
                if (it->callerTokenId == callerTokenId) {
                    it = value.erase(it);
                } else {
                    it++;
                }
            }
            return !value.empty();
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

int PublishedDataSubscriberManager::DisableSubscriber(const std::string &uri, const std::string &callerBundleName,
    const int64_t subscriberId, const uint32_t callerTokenId)
{
    PublishedDataKey key(uri, callerBundleName, subscriberId);
    auto result =
        publishedDataCache.ComputeIfPresent(key, [&callerTokenId](const auto &key, std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end(); it++) {
                if (it->callerTokenId == callerTokenId) {
                    it->enabled = false;
                }
            }
            return true;
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

int PublishedDataSubscriberManager::EnableSubscriber(const std::string &uri, const std::string &callerBundleName,
    const int64_t subscriberId, const uint32_t callerTokenId)
{
    PublishedDataKey key(uri, callerBundleName, subscriberId);
    auto result =
        publishedDataCache.ComputeIfPresent(key, [&callerTokenId](const auto &key, std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end(); it++) {
                if (it->callerTokenId == callerTokenId) {
                    it->enabled = true;
                }
            }
            return true;
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

void PublishedDataSubscriberManager::Emit(const std::vector<PublishedDataKey> &keys,
    const std::string &ownerBundleName, const sptr<IDataProxyPublishedDataObserver> observer)
{
    int32_t status;
    // key is bundleName, value is change node
    std::map<PublishedDataKey, std::variant<std::vector<uint8_t>, std::string>> publishedResult;
    std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> callbacks;
    publishedDataCache.ForEach([&keys, &status, &observer, &publishedResult, &callbacks, this](
                                   const PublishedDataKey &key, std::vector<ObserverNode> &val) {
        for (auto &data : keys) {
            if (key != data || publishedResult.count(key) != 0) {
                continue;
            }
            status = PublishedData::Query(
                Id(PublishedData::GenId(key.key, key.bundleName, key.subscriberId)), publishedResult[key]);
            if (status != E_OK) {
                ZLOGE("query fail %{public}s %{public}s %{public}" PRId64, data.bundleName.c_str(), data.key.c_str(),
                    data.subscriberId);
                publishedResult.erase(key);
                continue;
            }
            PutInto(callbacks, val, key, observer);
            break;
        }
        return false;
    });
    PublishedDataChangeNode result;
    for (auto &[callback, keys] : callbacks) {
        result.datas_.clear();
        for (auto &key : keys) {
            if (publishedResult.count(key) != 0) {
                result.datas_.emplace_back(key.key, key.subscriberId, publishedResult[key]);
            }
        }
        if (result.datas_.empty()) {
            continue;
        }
        result.ownerBundleName_ = ownerBundleName;
        callback->OnChangeFromPublishedData(result);
    }
}

void PublishedDataSubscriberManager::PutInto(
    std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> &callbacks,
    std::vector<ObserverNode> &val, const PublishedDataKey &key, const sptr<IDataProxyPublishedDataObserver> observer)
{
    for (auto &callback : val) {
        if (callback.enabled && callback.observer != nullptr) {
            // callback the observer, others do not call
            if (observer != nullptr && callback.observer != observer) {
                continue;
            }
            callbacks[callback.observer].emplace_back(key);
        }
    }
}

PublishedDataKey::PublishedDataKey(const std::string &key, const std::string &bundle, const int64_t subscriberId)
    : key(key), bundleName(bundle), subscriberId(subscriberId)
{
    /* private published data can use key as simple uri */
    /* etc: datashareproxy://{bundleName}/meeting can use meeting replaced */
    /* if key is normal uri, bundleName is from uri */
    if (URIUtils::IsDataProxyURI(key)) {
        URIUtils::GetBundleNameFromProxyURI(key, bundleName);
    }
}

bool PublishedDataKey::operator<(const PublishedDataKey &rhs) const
{
    if (key < rhs.key) {
        return true;
    }
    if (rhs.key < key) {
        return false;
    }
    if (bundleName < rhs.bundleName) {
        return true;
    }
    if (rhs.bundleName < bundleName) {
        return false;
    }
    return subscriberId < rhs.subscriberId;
}

bool PublishedDataKey::operator>(const PublishedDataKey &rhs) const
{
    return rhs < *this;
}

bool PublishedDataKey::operator<=(const PublishedDataKey &rhs) const
{
    return !(rhs < *this);
}

bool PublishedDataKey::operator>=(const PublishedDataKey &rhs) const
{
    return !(*this < rhs);
}

bool PublishedDataKey::operator==(const PublishedDataKey &rhs) const
{
    return key == rhs.key && bundleName == rhs.bundleName && subscriberId == rhs.subscriberId;
}

bool PublishedDataKey::operator!=(const PublishedDataKey &rhs) const
{
    return !(rhs == *this);
}

RdbSubscriberManager::ObserverNode::ObserverNode(const sptr<IDataProxyRdbObserver> &observer, uint32_t callerTokenId)
    : observer(observer), callerTokenId(callerTokenId)
{
}

PublishedDataSubscriberManager::ObserverNode::ObserverNode(
    const sptr<IDataProxyPublishedDataObserver> &observer, uint32_t callerTokenId)
    : observer(observer), callerTokenId(callerTokenId)
{
}
} // namespace OHOS::DataShare