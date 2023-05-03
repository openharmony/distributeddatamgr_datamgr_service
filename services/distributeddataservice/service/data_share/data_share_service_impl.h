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

#include "data_share_service_stub.h"
#include "datashare_template.h"
#include "data_proxy_observer.h"
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
    Data GetData(const std::string &bundleNameOfProvider) override;
    std::vector<OperationResult> SubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &id, const sptr<IDataProxyRdbObserver> observer) override;
    std::vector<OperationResult> UnSubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &id) override;
    std::vector<OperationResult> EnableSubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &id) override;
    std::vector<OperationResult> DisableSubscribeRdbData(
        const std::vector<std::string> &uris, const TemplateId &id) override;
    std::vector<OperationResult> SubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId, const sptr<IDataProxyPublishedDataObserver> observer) override;
    std::vector<OperationResult> UnSubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId) override;
    std::vector<OperationResult> EnableSubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId) override;
    std::vector<OperationResult> DisableSubscribePublishedData(const std::vector<std::string> &uris,
        const int64_t subscriberId) override;
    int32_t OnInitialize() override;
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override;

private:
    class Factory {
    public:
        Factory();
        ~Factory();
    };

    enum class PermissionType {
        READ_PERMISSION = 1,
        WRITE_PERMISSION
    };

    bool NotifyChange(const std::string &uri);
    bool GetCallerBundleName(std::string &bundleName);
    static Factory factory_;
    static constexpr int32_t ERROR = -1;
};
} // namespace OHOS::DataShare
#endif
