/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_EVENT_CENTER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_EVENT_CENTER_H
#include <list>
#include <queue>
#include <memory>
#include "concurrent_map.h"
#include "eventcenter/event.h"
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class EventCenter {
public:
    enum : int32_t {
        CODE_SYNC = 1,
        CODE_ASYNC,
        CODE_INVALID_ARGS,
    };

    class Defer final {
    public:
        API_EXPORT Defer(std::function<void(const Event &)> handler = nullptr, int32_t evtId = Event::EVT_INVALID);
        API_EXPORT ~Defer();
        void *operator new (size_t size) = delete;
        void *operator new[] (size_t size) = delete;
        void operator delete (void *) = delete;
        void operator delete[] (void *) = delete;
    };
    API_EXPORT static EventCenter &GetInstance();
    API_EXPORT bool Subscribe(int32_t evtId, const std::function<void(const Event &)> &observer);
    API_EXPORT bool Unsubscribe(int32_t evtId);
    API_EXPORT int32_t PostEvent(std::unique_ptr<Event> evt) const;
private:
    void Dispatch(const Event &evt) const;
    class AsyncQueue final {
    public:
        static constexpr int32_t MAX_CAPABILITY = 100;
        AsyncQueue &operator++();
        AsyncQueue &operator--();
        bool operator<=(int32_t depth) const;
        void Post(std::unique_ptr<Event> event);
        void AddHandler(int32_t evtId, std::function<void(const Event &)> handler);
    private:
        std::map<int32_t, std::function<void(const Event &)>> handlers_;
        std::deque<std::unique_ptr<Event>> events_;
        int32_t depth_ = 0;
    };
    ConcurrentMap<int32_t, std::list<std::function<void(const Event &)>>> observers_;
    static thread_local AsyncQueue *asyncQueue_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_EVENT_CENTER_H
