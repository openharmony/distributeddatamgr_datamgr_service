/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "visibility.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
class API_EXPORT DataShareServiceImpl : public DataShareServiceStub {
public:
    DataShareServiceImpl() = default;
    int32_t Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket) override;
    int32_t Update(const std::string &uri, const DataSharePredicates &predicate,
                   const DataShareValuesBucket &valuesBucket) override;
    int32_t Delete(const std::string &uri, const DataSharePredicates &predicate) override;
    std::shared_ptr<DataShareResultSet> Query(const std::string &uri, const DataSharePredicates &predicates,
                                              const std::vector<std::string> &columns) override;
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
    bool CheckPermisson(const UriInfo &uriInfo, PermissionType permissionType);
    static Factory factory_;
    static constexpr int32_t ERROR = -1;
};
} // namespace OHOS::DataShare
#endif
