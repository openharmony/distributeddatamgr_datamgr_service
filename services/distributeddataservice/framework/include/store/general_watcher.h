/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_WATCHER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_WATCHER_H
#include <cstdint>

#include "store/general_value.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class GeneralWatcher {
public:
    struct Origin {
        enum OriginType : int32_t {
            ORIGIN_LOCAL,
            ORIGIN_NEARBY,
            ORIGIN_CLOUD,
            ORIGIN_ALL,
            ORIGIN_BUTT,
        };
        enum DataType : int32_t {
            BASIC_DATA,
            ASSET_DATA,
            TYPE_BUTT,
        };
        int32_t origin = ORIGIN_BUTT;
        int32_t dataType = BASIC_DATA;
        // origin is ORIGIN_LOCAL, the id is empty
        // origin is ORIGIN_NEARBY, the id is networkId;
        // origin is ORIGIN_CLOUD, the id is the cloud account id
        std::vector<std::string> id;
        std::string store;
    };
    enum ChangeOp : int32_t {
        OP_INSERT,
        OP_UPDATE,
        OP_DELETE,
        OP_BUTT,
    };
    // PK primary key
    using PRIValue = std::variant<std::monostate, std::string, int64_t, double>;
    using PRIFields = std::map<std::string, std::string>;
    using Fields = std::map<std::string, std::vector<std::string>>;
    using ChangeInfo = std::map<std::string, std::vector<PRIValue>[OP_BUTT]>;
    using ChangeData = std::map<std::string, std::vector<Values>[OP_BUTT]>;
    virtual ~GeneralWatcher() = default;
    virtual int32_t OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values) = 0;
    virtual int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_WATCHER_H
