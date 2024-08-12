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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_REMOTE_CHANGE_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_REMOTE_CHANGE_EVENT_H

#include <cstdint>
#include <string>

#include "eventcenter/event.h"

namespace OHOS::DistributedData {
class API_EXPORT RemoteChangeEvent : public Event {
public:
    enum : int32_t {
        RDB_META_SAVE = EVT_CHANGE,
        DATA_CHANGE,
    };

    struct DataInfo {
        std::string userId;
        std::string storeId;
        std::string deviceId;
        std::string bundleName;
        int changeType = 0; // 0 means CLOUD_DATA_CHANGE
        std::vector<std::string> tables;
    };

    RemoteChangeEvent(int32_t evtId, DataInfo&& info);

    ~RemoteChangeEvent() = default;

    const DataInfo& GetDataInfo() const;

private:
    DataInfo info_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_REMOTE_CHANGE_EVENT_H