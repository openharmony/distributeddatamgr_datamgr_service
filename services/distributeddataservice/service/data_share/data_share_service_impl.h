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

#ifndef DATASHARESERVICE_DATA_SERVICE_IMPL_H
#define DATASHARESERVICE_DATA_SERVICE_IMPL_H

#include <string>

#include "common_event_subscribe_info.h"
#include "common_event_subscriber.h"
#include "data_proxy_observer.h"
#include "data_share_service_stub.h"
#include "datashare_template.h"
#include "db_delegate.h"
#include "delete_strategy.h"
#include "feature/static_acts.h"
#include "get_data_strategy.h"
#include "insert_strategy.h"
#include "publish_strategy.h"
#include "query_strategy.h"
#include "rdb_notify_strategy.h"
#include "subscribe_strategy.h"
#include "template_strategy.h"
#include "update_strategy.h"
#include "uri_utils.h"
#include "visibility.h"

namespace OHOS::DataShare {
class API_EXPORT DataShareServiceImpl : public DataShareServiceStub {
public:
    DataShareServiceImpl() = default;
    int32_t Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket) override;
    int32_t Update(const std::string &uri, const DataSharePredicates &predicate,
                   const DataShareValuesBucket &valuesBucket) override;
    int32_t Delete(const std::string &uri, const DataSharePredicates &predicate) override;
    std::shared_ptr<DataShareResultSet> Query(const std::string &uri, const DataSharePredicates &predicates,
                                              const std::vector<std::string> &columns, int &errCode) override;
    int32_t AddTemplate(const std::string &uri, const int64_t subscriberId, const Template &tplt) override;
    int32_t DelTemplate(const std::string &uri, const int64_t subscriberId) override;
    std::vector<OperationResult> Publish(const Data &data, const std::string &bundleNameOfProvider) override;
    Data GetData(const std::string &bundleNameOfProvider, int &errorCode) override;
    std::vector<OperationResult> SubscribeRdbData(const std::vector<std::string> &uris,
        const TemplateId &id, const sptr<IDataProxyRdbObserver> observer) override;
    std::vector<OperationResult> UnsubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &id) override;
    std::vector<OperationResult> EnableRdbSubs(
        const std::vector<std::string> &uris, const TemplateId &id) override;
    std::vector<OperationResult> DisableRdbSubs(
        const std::vector<std::string> &uris, const TemplateId &id) override;
    std::vector<OperationResult> SubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId, const sptr<IDataProxyPublishedDataObserver> observer) override;
    std::vector<OperationResult> UnsubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId) override;
    std::vector<OperationResult> EnablePubSubs(const std::vector<std::string> &uris,
        const int64_t subscriberId) override;
    std::vector<OperationResult> DisablePubSubs(const std::vector<std::string> &uris,
        const int64_t subscriberId) override;
    void OnConnectDone() override;
    int32_t OnBind(const BindInfo &binderInfo) override;
    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName) override;
    void NotifyObserver(const std::string &uri) override;

private:
    using StaticActs = DistributedData::StaticActs;
    class DataShareStatic : public StaticActs {
    public:
        ~DataShareStatic() override {};
        int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index) override;
    };
    class Factory {
    public:
        Factory();
        ~Factory();
    private:
        std::shared_ptr<DataShareStatic> staticActs_;
    };
    class TimerReceiver : public EventFwk::CommonEventSubscriber {
    public:
        TimerReceiver() = default;
        explicit TimerReceiver(const EventFwk::CommonEventSubscribeInfo &subscriberInfo);
        virtual ~TimerReceiver() = default;
        virtual void OnReceiveEvent(const EventFwk::CommonEventData &eventData) override;
    };
    bool SubscribeTimeChanged();
    bool NotifyChange(const std::string &uri);
    bool GetCallerBundleName(std::string &bundleName);
    static Factory factory_;
    static constexpr int32_t ERROR = -1;
    PublishStrategy publishStrategy_;
    GetDataStrategy getDataStrategy_;
    SubscribeStrategy subscribeStrategy_;
    DeleteStrategy deleteStrategy_;
    InsertStrategy insertStrategy_;
    QueryStrategy queryStrategy_;
    UpdateStrategy updateStrategy_;
    TemplateStrategy templateStrategy_;
    RdbNotifyStrategy rdbNotifyStrategy_;
    BindInfo binderInfo_;
    std::shared_ptr<TimerReceiver> timerReceiver_ = nullptr;
};
} // namespace OHOS::DataShare
#endif
