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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_STORE_MESSAGE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_STORE_MESSAGE_H

#include <string>

#include "eventcenter/message.h"
#include "store/general_value.h"

namespace OHOS::DistributedData {
class API_EXPORT StoreMessage : public Message {
public:
    enum : int32_t {
        FEATURE_INIT = MSG_CLOUD,
        GET_CALLBACK,
        FEATURE_BUTT
    };

    struct StoreInfo {
        int32_t user = 0;
        std::string bundleName;
        int32_t instanceId = 0;
        std::string storeName;
    };

    StoreMessage(int32_t messageId, StoreInfo storeInfo);
    StoreMessage(int32_t messageId);
    const StoreInfo& GetStoreInfo() const;
    void SetAsync(GenAsync&& async);
    GenAsync GetAsync();
    ~StoreMessage() = default;

private:
    StoreInfo storeInfo_;
    GenAsync async_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_STORE_MESSAGE_H
