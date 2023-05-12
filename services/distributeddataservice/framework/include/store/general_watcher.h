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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_WATCHER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_WATCHER_H
#include <cstdint>
#include "store/general_value.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class GeneralWatcher {
public:
    enum Origin : int32_t {
        ORIGIN_CLOUD,
        ORIGIN_LOCAL,
        ORIGIN_REMOTE,
        ORIGIN_ALL,
        ORIGIN_BUTT,
    };

    enum ChangeOp : int32_t {
        OP_INSERT,
        OP_UPDATE,
        OP_DELETE,
        OP_BUTT,
    };

    virtual ~GeneralWatcher() = default;
    virtual int32_t OnChange(Origin origin, const std::string &id) = 0;
    virtual int32_t OnChange(Origin origin, const std::string &id, const std::vector<VBucket> &values) = 0;
};
}
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_WATCHER_H
