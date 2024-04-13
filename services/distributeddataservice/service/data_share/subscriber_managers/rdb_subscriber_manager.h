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

#ifndef DATASHARESERVICE_RDB_SUBSCRIBER_MANAGER_H
#define DATASHARESERVICE_RDB_SUBSCRIBER_MANAGER_H

#include <list>
#include <string>

#include "concurrent_map.h"
#include "context.h"
#include "data_provider_config.h"
#include "data_proxy_observer.h"
#include "data_share_db_config.h"
#include "datashare_template.h"
#include "executor_pool.h"

namespace OHOS::DataShare {
struct Key {
    Key(const std::string &uri, int64_t subscriberId, const std::string &bundleName);
    bool operator==(const Key &rhs) const;
    bool operator!=(const Key &rhs) const;
    bool operator<(const Key &rhs) const;
    bool operator>(const Key &rhs) const;
    bool operator<=(const Key &rhs) const;
    bool operator>=(const Key &rhs) const;
    const std::string uri;
    const int64_t subscriberId;
    const std::string bundleName;
};
class TemplateManager {
public:
    static TemplateManager &GetInstance();
    int32_t Add(const Key &key, int32_t userId, const Template &tpl);
    int32_t Delete(const Key &key, int32_t userId);
    bool Get(const Key &key, int32_t userId, Template &tpl);

private:
    TemplateManager();
    friend class RdbSubscriberManager;
};

class RdbSubscriberManager {
public:
    static RdbSubscriberManager &GetInstance();
    int Add(const Key &key, const sptr<IDataProxyRdbObserver> observer, std::shared_ptr<Context> context,
        std::shared_ptr<ExecutorPool> executorPool);
    int Delete(const Key &key, uint32_t firstCallerTokenId);
    void Delete(uint32_t callerTokenId);
    int Disable(const Key &key, uint32_t firstCallerTokenId);
    int Enable(const Key &key, std::shared_ptr<Context> context);
    void Emit(const std::string &uri, int64_t subscriberId, std::shared_ptr<Context> context);
    void Emit(const std::string &uri, std::shared_ptr<Context> context);
    void Emit(const std::string &uri, int32_t userId, DistributedData::StoreMetaData &metaData);
    void EmitByKey(const Key &key, int32_t userId, const std::string &rdbPath, int version);
    std::vector<Key> GetKeysByUri(const std::string &uri);
    void Clear();

private:
    struct ObserverNode {
        ObserverNode(const sptr<IDataProxyRdbObserver> &observer, uint32_t firstCallerTokenId,
            uint32_t callerTokenId = 0);
        sptr<IDataProxyRdbObserver> observer;
        uint32_t firstCallerTokenId;
        uint32_t callerTokenId;
        bool enabled = true;
        bool isNotifyOnEnabled = false;
    };

    RdbSubscriberManager() = default;
    ConcurrentMap<Key, std::vector<ObserverNode>> rdbCache_;
    int Notify(const Key &key, int32_t userId, const std::vector<ObserverNode> &val, const std::string &rdbDir,
        int rdbVersion);
    int GetEnableObserverCount(const Key &key);
    void SetObserverNotifyOnEnabled(std::vector<ObserverNode> &nodes);
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_RDB_SUBSCRIBER_MANAGER_H
