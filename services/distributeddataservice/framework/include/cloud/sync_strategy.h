/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SYNC_STRATEGY_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SYNC_STRATEGY_H
#include "serializable/serializable.h"
#include "store/store_info.h"
namespace OHOS::DistributedData {
class API_EXPORT SyncStrategy {
public:
    virtual ~SyncStrategy() = default;
    virtual int32_t CheckSyncAction(const StoreInfo& storeInfo);
    virtual std::shared_ptr<SyncStrategy> SetNext(std::shared_ptr<SyncStrategy> next);
    static constexpr const char *GLOBAL_BUNDLE = "GLOBAL";
private:
    std::shared_ptr<SyncStrategy> next_;
};
}
#endif //OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SYNC_STRATEGY_H
