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

#ifndef DISTRIBUTEDDATAMGR_OBJECT_DMS_HANDLER_H
#define DISTRIBUTEDDATAMGR_OBJECT_DMS_HANDLER_H

#include <deque>

#include "dms_listener_stub.h"
#include "distributed_sched_types.h"

namespace OHOS::DistributedObject {
class DmsEventListener : public DistributedSchedule::DSchedEventListenerStub {
public:
    DmsEventListener() = default;
    ~DmsEventListener() = default;
    void DSchedEventNotify(DistributedSchedule::EventNotify &notify);
};

class ObjectDmsHandler {
public:
    static ObjectDmsHandler &GetInstance()
    {
        static ObjectDmsHandler instance;
        return instance;
    }

    void ReceiveDmsEvent(DistributedSchedule::EventNotify &event);
    bool IsContinue(const std::string &networkId, const std::string &bundleName);
    void RegisterDmsEvent();

private:
    ObjectDmsHandler() = default;
    ~ObjectDmsHandler() = default;
    ObjectDmsHandler(const ObjectDmsHandler &) = delete;
    ObjectDmsHandler &operator=(const ObjectDmsHandler &) = delete;
    ObjectDmsHandler(ObjectDmsHandler &&) = delete;
    ObjectDmsHandler &operator=(ObjectDmsHandler &&) = delete;

    static constexpr int64_t VALIDITY = 15;
    static constexpr uint32_t MAX_EVENTS = 20;
    std::deque<std::pair<DistributedSchedule::EventNotify, std::chrono::steady_clock::time_point>> dmsEvents_{};
    std::mutex mutex_{};
    sptr<DistributedSchedule::IDSchedEventListener> dmsEventListener_;
};
} // namespace OHOS::DistributedObject
#endif // DISTRIBUTEDDATAMGR_OBJECT_DMS_HANDLER_H
