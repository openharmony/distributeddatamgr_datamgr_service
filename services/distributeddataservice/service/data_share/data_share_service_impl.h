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

#include <cstdint>
#include <functional>
#include <string>

#include "accesstoken_kit.h"
#include "bundle_mgr_proxy.h"
#include "changeevent/remote_change_event.h"
#include "common_event_subscribe_info.h"
#include "common_event_subscriber.h"
#include "data_provider_config.h"
#include "data_proxy_observer.h"
#include "data_share_db_config.h"
#include "data_share_service_stub.h"
#include "data_share_silent_config.h"
#include "datashare_template.h"
#include "db_delegate.h"
#include "eventcenter/event.h"
#include "feature/static_acts.h"
#include "get_data_strategy.h"
#include "publish_strategy.h"
#include "rdb_notify_strategy.h"
#include "subscribe_strategy.h"
#include "unsubscribe_strategy.h"
#include "template_strategy.h"
#include "utils.h"
#include "visibility.h"

namespace OHOS::DataShare {
class DataShareServiceImpl : public DataShareServiceStub {
public:
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;
    using ExecuteCallbackEx = std::function<std::pair<int32_t, int32_t>(DataProviderConfig::ProviderInfo &,
        DistributedData::StoreMetaData &, std::shared_ptr<DBDelegate>)>;
    DataShareServiceImpl() = default;
    virtual ~DataShareServiceImpl();
    std::shared_ptr<DataShareResultSet> Query(const std::string &uri, const std::string &extUri,
        const DataSharePredicates &predicates, const std::vector<std::string> &columns, int &errCode) override;
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
    int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index) override;
    int32_t OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index) override;
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override;
    void NotifyObserver(const std::string &uri) override;
    static void DumpDataShareServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params);
    int32_t OnInitialize() override;
    int32_t EnableSilentProxy(const std::string &uri, bool enable) override;
    int32_t GetSilentProxyStatus(const std::string &uri, bool isCreateHelper) override;
    int32_t RegisterObserver(const std::string &uri, const sptr<OHOS::IRemoteObject> &remoteObj) override;
    int32_t UnregisterObserver(const std::string &uri, const sptr<OHOS::IRemoteObject> &remoteObj) override;
    std::pair<int32_t, int32_t> InsertEx(const std::string &uri, const std::string &extUri,
        const DataShareValuesBucket &valuesBucket) override;
    std::pair<int32_t, int32_t> UpdateEx(const std::string &uri, const std::string &extUri,
        const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket) override;
    std::pair<int32_t, int32_t> DeleteEx(const std::string &uri, const std::string &extUri,
        const DataSharePredicates &predicate) override;
    std::vector<DataProxyResult> PublishProxyData(const std::vector<DataShareProxyData> &proxyData,
        const DataProxyConfig &proxyConfig) override;
    std::vector<DataProxyResult> DeleteProxyData(const std::vector<std::string> &uris,
        const DataProxyConfig &proxyConfig) override;
    std::vector<DataProxyGetResult> GetProxyData(const std::vector<std::string> &uris,
        const DataProxyConfig &proxyConfig) override;
    std::vector<DataProxyResult> SubscribeProxyData(const std::vector<std::string> &uris,
        const DataProxyConfig &proxyConfig, const sptr<IProxyDataObserver> observer) override;
    std::vector<DataProxyResult> UnsubscribeProxyData(const std::vector<std::string> &uris,
        const DataProxyConfig &proxyConfig) override;
    static void UpdateLaunchInfo();
private:
    class SystemAbilityStatusChangeListener;
    using StaticActs = DistributedData::StaticActs;
    class DataShareStatic : public StaticActs {
    public:
        ~DataShareStatic() override {};
        int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index,
            int32_t tokenId = -1) override;
        int32_t OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index) override;
        int32_t OnClearAppStorage(const std::string &bundleName, int32_t user, int32_t index, int32_t tokenId) override;
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
        void OnReceiveEvent(const EventFwk::CommonEventData &eventData) override;
    };
    void RegisterDataShareServiceInfo();
    void RegisterHandler();
    void SubscribeListen();
    bool SubscribeTimeChanged();
    bool NotifyChange(const std::string &uri, int32_t userId);
    bool GetCallerBundleName(std::string &bundleName);
    std::pair<int32_t, int32_t> ExecuteEx(const std::string &uri, const std::string &extUri, const int32_t tokenId,
        bool isRead, ExecuteCallbackEx callback);
    std::pair<bool, Security::AccessToken::ATokenTypeEnum> GetCallerInfo(std::string &bundleName, int32_t &appIndex);
    int32_t GetBMSAndMetaDataStatus(const std::string &uri, const int32_t tokenId);
    void SubscribeCommonEvent();
    void SubscribeConcurrentTask();
    static void InitSubEvent();
    void AutoLaunch(const DistributedData::Event &event);
    void SubscribeChange();
    bool AllowCleanDataLaunchApp(const DistributedData::Event &event, bool launchForCleanData);
    static void SaveLaunchInfo(const std::string &bundleName, const std::string &userId,
        const std::string &deviceId);
    static DistributedData::StoreMetaData MakeMetaData(const std::string &bundleName, const std::string &userId,
        const std::string &deviceId, const std::string storeId = "");
    bool VerifyAcrossAccountsPermission(int32_t currentUserId, int32_t visitedUserId,
        const std::string &acrossAccountsPermission, uint32_t callerTokenId);
    void ReportExcuteFault(uint32_t callingTokenId, DataProviderConfig::ProviderInfo &providerInfo,
        int32_t errCode, std::string &func);
    bool CheckAllowList(const uint32_t &currentUserId, const uint32_t &callerTokenId,
        const std::vector<AllowList> &allowLists);
    bool VerifyPermission(const std::string &bundleName, const std::string &permission,
        bool isFromExtension, const int32_t tokenId);
    bool GetCallerBundleInfo(BundleInfo &callerBundleInfo);
    void SetCriticalTask();
    static Factory factory_;
    static constexpr int32_t ERROR = -1;
    static constexpr int32_t ERROR_PERMISSION_DENIED = -2;
    static constexpr const char *PROXY_URI_SCHEMA = "datashareproxy";
    static constexpr const char *EXT_URI_SCHEMA = "datashare://";
    static constexpr const char *NO_PERMISSION = "noPermission";
    PublishStrategy publishStrategy_;
    GetDataStrategy getDataStrategy_;
    SubscribeStrategy subscribeStrategy_;
    UnsubscribeStrategy unsubscribeStrategy_;
    TemplateStrategy templateStrategy_;
    RdbNotifyStrategy rdbNotifyStrategy_;
    static BindInfo binderInfo_;
    std::shared_ptr<TimerReceiver> timerReceiver_ = nullptr;
    DataShareSilentConfig dataShareSilentConfig_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_DATA_SERVICE_IMPL_H
