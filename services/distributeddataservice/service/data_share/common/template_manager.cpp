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
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return false;
    }
    TemplateData data(uri, bundleName, subscriberId, tpl);
    std::string result;
    bool find = delegate->Get(KvDBDelegate::TEMPLATE_TABLE, *data.GetId(), result);
    if (!find) {
        ZLOGE("db Get failed, %{public}s, %{public}" PRId64, bundleName.c_str(), subscriberId);
        return false;
    }
    if (!DistributedData::Serializable::Unmarshall(result, data.value)) {
        ZLOGE("Unmarshall failed, %{public}s, %{public}" PRId64, bundleName.c_str(), subscriberId);
        return false;
    }
    tpl = data.value.tpl.tpl;
    return true;
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
    Template tpl;
    TemplateData data(uri, tplId.bundleName_, tplId.subscriberId_, tpl);
    auto status = delegate->DeleteById(KvDBDelegate::TEMPLATE_TABLE, *data.GetId());
    if (status != E_OK) {
        ZLOGE("db DeleteById failed, %{public}d", status);
        return false;
    }
    SchedulerManager::GetInstance().RemoveTimer(Key(uri, tplId.subscriberId_, tplId.bundleName_));
    return true;
}

Key::Key(const std::string &uri, const int64_t subscriberId, const std::string &bundleName)
    : uri_(uri), subscriberId_(subscriberId), bundleName_(bundleName)
{
}

bool Key::operator==(const Key &rhs) const
{
    return uri_ == rhs.uri_ && subscriberId_ == rhs.subscriberId_ && bundleName_ == rhs.bundleName_;
}

bool Key::operator!=(const Key &rhs) const
{
    return !(rhs == *this);
}
bool Key::operator<(const Key &rhs) const
{
    if (uri_ < rhs.uri_)
        return true;
    if (rhs.uri_ < uri_)
        return false;
    if (subscriberId_ < rhs.subscriberId_)
        return true;
    if (rhs.subscriberId_ < subscriberId_)
        return false;
    return bundleName_ < rhs.bundleName_;
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
    const sptr<IDataProxyRdbObserver> observer, std::shared_ptr<Context> context)
{
    int result = E_OK;
    Key key(uri, tplId.subscriberId_, tplId.bundleName_);
    rdbCache_.Compute(
        key, [&observer, &context, &result, this](const auto &key, std::vector<ObserverNode> &value) {
            ZLOGI("add subscriber, uri %{private}s tokenId %{public}d", key.uri_.c_str(), context->callerTokenId);
            ObserverNode observerNode(observer, context->callerTokenId);
            std::vector<ObserverNode> node({ observerNode });
            result = Notify(key, node, context->calledSourceDir, context->version);
            if (result != E_OK) {
                return false;
            }
            value.emplace_back(observerNode);
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
    auto result = rdbCache_.ComputeIfPresent(key, [&callerTokenId, this](const auto &key, std::vector<ObserverNode> &value) {
        ZLOGI("delete subscriber, uri %{public}s tokenId %{public}d", key.uri_.c_str(), callerTokenId);
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
    auto result = rdbCache_.ComputeIfPresent(key, [&callerTokenId, this](const auto &key, std::vector<ObserverNode> &value) {
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

int RdbSubscriberManager::EnableRdbSubscriber(const std::string &uri, const TemplateId &tplId, std::shared_ptr<Context> context)
{
    Key key(uri, tplId.subscriberId_, tplId.bundleName_);
    auto result = rdbCache_.ComputeIfPresent(key, [&context, this](const auto &key, std::vector<ObserverNode> &value) {
        for (auto it = value.begin(); it != value.end(); it++) {
            if (it->callerTokenId == context->callerTokenId) {
                it->enabled = true;
                ObserverNode observerNode(it->observer, context->callerTokenId);
                std::vector<ObserverNode> node({ observerNode });
                Notify(key, node, context->calledSourceDir, context->version);
                if (GetEnableObserverCount(key) == 1) {
                    SchedulerManager::GetInstance().Execute(key, context->calledSourceDir, context->version);
                }
            }
        }
        return true;
    });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

class JsonFormatter : public DistributedData::Serializable {
public:
    JsonFormatter(const std::string &key, const std::shared_ptr<DistributedData::Serializable> &value)
        : key_(key), value_(value)
    {
    }
    bool Marshal(json &node) const override
    {
        if (value_ == nullptr) {
            ZLOGE("null value %{public}s", key_.c_str());
            return false;
        }
        // ZLOGE("key %{public}s value %{public}s", key_.c_str(), DistributedData::Serializable::Marshall(value_).c_str());
        return SetValue(node[key_], *value_);
    }
    bool Unmarshal(const json &node) override
    {
        if (value_ == nullptr) {
            ZLOGE("null value %{public}s", key_.c_str());
            return false;
        }
        return GetValue(node, key_, *value_);
    }

private:
    std::string key_;
    std::shared_ptr<DistributedData::Serializable> value_;
};

void RdbSubscriberManager::Emit(const std::string &uri, std::shared_ptr<Context> context)
{
    if (!URIUtils::IsDataProxyURI(uri)) {
        return;
    }
    rdbCache_.ForEach([&uri, &context, this](const Key &key, std::vector<ObserverNode> &val) {
        if (key.uri_ != uri) {
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
        if (key.uri_ != uri) {
            return false;
        }
        results.emplace_back(key);
        return false;
    });
    return results;
}

void RdbSubscriberManager::EmitByKey(const Key &key, const std::string &rdbPath, int version)
{
    if (!URIUtils::IsDataProxyURI(key.uri_)) {
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
    const Key &key, std::vector<ObserverNode> &val, const std::string &rdbDir, int rdbVersion)
{
    Template tpl;
    if (!TemplateManager::GetInstance().GetTemplate(key.uri_, key.subscriberId_, key.bundleName_, tpl)) {
        ZLOGE("template undefined, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri_).c_str(), key.subscriberId_, key.bundleName_.c_str());
        return E_TEMPLATE_NOT_EXIST;
    }
    int errCode;
    auto delegate = DBDelegate::Create(rdbDir, rdbVersion, errCode);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s %{public}s", DistributedData::Anonymous::Change(key.uri_).c_str(),
            key.bundleName_.c_str());
        return errCode;
    }
    RdbChangeNode changeNode;
    changeNode.uri_ = key.uri_;
    changeNode.templateId_.subscriberId_ = key.subscriberId_;
    changeNode.templateId_.bundleName_ = key.bundleName_;
    for (const auto &predicate : tpl.predicates_) {
        JsonFormatter formatter(predicate.key_, delegate->Query(predicate.selectSql_));
        changeNode.data_.emplace_back(DistributedData::Serializable::Marshall(formatter));
    }

    ZLOGI("emit, size %{public}d %{private}s", val.size(), changeNode.uri_.c_str());
    for (auto &callback : val) {
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
    publishedDataCache_.Compute(
        publishedDataKey, [&observer, &callerTokenId, this](const PublishedDataKey &key, std::vector<ObserverNode> &value) {
            ZLOGI("add publish subscriber, uri %{private}s tokenId %{public}d", key.key_.c_str(), callerTokenId);
            value.emplace_back(observer, callerTokenId);
            return true;
        });
    return E_OK;
}

int PublishedDataSubscriberManager::DelSubscriber(const std::string &uri, const std::string &callerBundleName,
    const int64_t subscriberId, const uint32_t callerTokenId)
{
    PublishedDataKey key(uri, callerBundleName, subscriberId);
    auto result = publishedDataCache_.ComputeIfPresent(key, [&callerTokenId](const auto &key, std::vector<ObserverNode> &value) {
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
    auto result = publishedDataCache_.ComputeIfPresent(key, [&callerTokenId](const auto &key, std::vector<ObserverNode> &value) {
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
    auto result = publishedDataCache_.ComputeIfPresent(key, [&callerTokenId](const auto &key, std::vector<ObserverNode> &value) {
        for (auto it = value.begin(); it != value.end(); it++) {
            if (it->callerTokenId == callerTokenId) {
                it->enabled = true;
            }
        }
        return true;
    });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

void PublishedDataSubscriberManager::Emit(
    std::vector<PublishedDataKey> keys, const std::string &ownerBundleName, const sptr<IDataProxyPublishedDataObserver> observer)
{
    int32_t status;
    // key is bundleName, value is change node
    std::map<PublishedDataKey, PublishedDataItem> publishedResult;
    std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> callbacks;
    publishedDataCache_.ForEach([&keys, &status, &observer, &publishedResult, &callbacks](
                                    const PublishedDataKey &key, std::vector<ObserverNode> &val) {
        for (auto &data : keys) {
            ZLOGE("HANLU start try emit %{public}s %{public}s %{public}" PRId64, data.bundleName_.c_str(),
                data.key_.c_str(), data.subscriberId_);
            ZLOGE("HANLU observer %{public}s %{public}s %{public}" PRId64, key.bundleName_.c_str(), key.key_.c_str(),
                key.subscriberId_);
            if (key != data) {
                continue;
            }
            publishedResult[key].subscriberId_ = data.subscriberId_;
            publishedResult[key].key_ = data.key_;
            PublishedData publishedData(data.key_, data.bundleName_, data.subscriberId_);
            status = PublishedData::Query(DistributedData::Serializable::Marshall(*publishedData.GetId()), publishedResult[key].value_);
            if (status != E_OK) {
                ZLOGE("query fail %{public}s %{public}s %{public}" PRId64, data.bundleName_.c_str(), data.key_.c_str(),
                    data.subscriberId_);
                publishedResult.erase(key);
                continue;
            }
            break;
        }
        for (auto &callback : val) {
            if (callback.enabled && callback.observer != nullptr) {
                // callback the observer, others do not call
                if (observer != nullptr && callback.observer != observer) {
                    continue;
                }
                callbacks[callback.observer].emplace_back(key);
            }
        }
        return false;
    });
    PublishedDataChangeNode result;
    for (auto &[callback, keys] : callbacks) {
        result.datas_.clear();
        for (auto &key : keys) {
            if (publishedResult.count(key) != 0) {
                result.datas_.emplace_back(publishedResult[key]);
            }
        }
        if (result.datas_.empty()) {
            continue;
        }
        result.ownerBundleName_ = ownerBundleName;
        ZLOGE("HANLU emit start %{public}d", result.datas_.size());
        callback->OnChangeFromPublishedData(result);
    }
}

PublishedDataKey::PublishedDataKey(const std::string &key, const std::string &bundleName, const int64_t subscriberId)
    : key_(key), bundleName_(bundleName), subscriberId_(subscriberId)
{
    /* private published data can use key as simple uri */
    /* etc: datashareproxy://{bundleName}/meeting can use meeting replaced */
    /* if key is normal uri, bundleName is from uri */
    if (URIUtils::IsDataProxyURI(key_)) {
        URIUtils::GetBundleNameFromProxyURI(key_, bundleName_);
    }
}

bool PublishedDataKey::operator<(const PublishedDataKey &rhs) const
{
    if (key_ < rhs.key_)
        return true;
    if (rhs.key_ < key_)
        return false;
    if (bundleName_ < rhs.bundleName_)
        return true;
    if (rhs.bundleName_ < bundleName_)
        return false;
    return subscriberId_ < rhs.subscriberId_;
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
    return key_ == rhs.key_ && bundleName_ == rhs.bundleName_ && subscriberId_ == rhs.subscriberId_;
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