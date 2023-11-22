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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_RDB_SHARE_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_RDB_SHARE_EVENT_H
#include "cloud/cloud_event.h"
#include "store/general_value.h"
#include "store/cursor.h"
#include "visibility.h"
namespace OHOS {
namespace DistributedRdb {
struct PredicatesMemo;
}
namespace DistributedData {
class API_EXPORT QuerySharedResourceEvent : public CloudEvent {
public:
    using Callback = std::function<void(int32_t, std::shared_ptr<Cursor>)>;
    QuerySharedResourceEvent(StoreInfo storeInfo, std::shared_ptr<DistributedRdb::PredicatesMemo> predicates,
        const std::vector<std::string>& columns, Callback callback);
    ~QuerySharedResourceEvent() override = default;
    std::shared_ptr<DistributedRdb::PredicatesMemo> GetPredicates() const;
    std::vector<std::string> GetColumns() const;
    Callback GetCallback() const;

private:
    std::shared_ptr<DistributedRdb::PredicatesMemo> predicates_;
    std::vector<std::string> columns_;
    Callback callback_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_RDB_SHARE_EVENT_H
