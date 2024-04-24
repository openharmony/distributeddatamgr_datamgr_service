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

#ifndef DISTRIBUTEDDATAFWK_IDATA_SHARE_SERVICE_H
#define DISTRIBUTEDDATAFWK_IDATA_SHARE_SERVICE_H

#include <iremote_broker.h>
#include <string>

#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "datashare_values_bucket.h"
#include "datashare_template.h"
#include "data_proxy_observer.h"
#include "iremote_object.h"

namespace OHOS::DataShare {
class IDataShareService {
public:
    enum {
        DATA_SHARE_SERVICE_CMD_INSERT,
        DATA_SHARE_SERVICE_CMD_DELETE,
        DATA_SHARE_SERVICE_CMD_UPDATE,
        DATA_SHARE_SERVICE_CMD_QUERY,
        DATA_SHARE_SERVICE_CMD_ADD_TEMPLATE,
        DATA_SHARE_SERVICE_CMD_DEL_TEMPLATE,
        DATA_SHARE_SERVICE_CMD_PUBLISH,
        DATA_SHARE_SERVICE_CMD_GET_DATA,
        DATA_SHARE_SERVICE_CMD_SUBSCRIBE_RDB,
        DATA_SHARE_SERVICE_CMD_UNSUBSCRIBE_RDB,
        DATA_SHARE_SERVICE_CMD_ENABLE_SUBSCRIBE_RDB,
        DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_RDB,
        DATA_SHARE_SERVICE_CMD_SUBSCRIBE_PUBLISHED,
        DATA_SHARE_SERVICE_CMD_UNSUBSCRIBE_PUBLISHED,
        DATA_SHARE_SERVICE_CMD_ENABLE_SUBSCRIBE_PUBLISHED,
        DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_PUBLISHED,
        DATA_SHARE_SERVICE_CMD_NOTIFY,
        DATA_SHARE_SERVICE_CMD_NOTIFY_OBSERVERS,
        DATA_SHARE_SERVICE_CMD_SET_SILENT_SWITCH,
        DATA_SHARE_SERVICE_CMD_IS_SILENT_PROXY_ENABLE,
        DATA_SHARE_SERVICE_CMD_REGISTER_OBSERVER,
        DATA_SHARE_SERVICE_CMD_UNREGISTER_OBSERVER,
        DATA_SHARE_SERVICE_CMD_MAX
    };

    enum { DATA_SHARE_ERROR = -1, DATA_SHARE_OK = 0 };
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DataShare.IDataShareService");

    virtual int32_t Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket) = 0;
    virtual int32_t Update(
        const std::string &uri, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket) = 0;
    virtual int32_t Delete(const std::string &uri, const DataSharePredicates &predicate) = 0;
    virtual std::shared_ptr<DataShareResultSet> Query(const std::string &uri, const DataSharePredicates &predicates,
        const std::vector<std::string> &columns, int &errCode) = 0;
    virtual int32_t AddTemplate(const std::string &uri, const int64_t subscriberId, const Template &tplt) = 0;
    virtual int32_t DelTemplate(const std::string &uri, const int64_t subscriberId) = 0;
    virtual std::vector<OperationResult> Publish(const Data &data, const std::string &bundleNameOfProvider) = 0;
    virtual Data GetData(const std::string &bundleNameOfProvider, int &errorCode) = 0;
    virtual std::vector<OperationResult> SubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &id, const sptr<IDataProxyRdbObserver> observer) = 0;
    virtual std::vector<OperationResult> UnsubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &id) = 0;
    virtual std::vector<OperationResult> EnableRdbSubs(
        const std::vector<std::string> &uris, const TemplateId &id) = 0;
    virtual std::vector<OperationResult> DisableRdbSubs(
        const std::vector<std::string> &uris, const TemplateId &id) = 0;
    virtual std::vector<OperationResult> SubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId, const sptr<IDataProxyPublishedDataObserver> observer) = 0;
    virtual std::vector<OperationResult> UnsubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId) = 0;
    virtual std::vector<OperationResult> EnablePubSubs(const std::vector<std::string> &uris,
        const int64_t subscriberId) = 0;
    virtual std::vector<OperationResult> DisablePubSubs(const std::vector<std::string> &uris,
        const int64_t subscriberId) = 0;
    virtual void OnConnectDone() = 0;
    virtual void NotifyObserver(const std::string &uri) = 0;
    virtual int32_t EnableSilentProxy(const std::string &uri, bool enable) = 0;
    virtual bool IsSilentProxyEnable(const std::string &uri) = 0;
    virtual int32_t RegisterObserver(const std::string &uri, const sptr<OHOS::IRemoteObject> &remoteObj) = 0;
    virtual int32_t UnregisterObserver(const std::string &uri,
        const sptr<OHOS::IRemoteObject> &remoteObj) = 0;
};
} // namespace OHOS::DataShare
#endif // DISTRIBUTEDDATAFWK_IDATA_SHARE_SERVICE_H
