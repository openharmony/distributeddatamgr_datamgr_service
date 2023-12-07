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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_COMMUNICATION_STRATEGY_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_COMMUNICATION_STRATEGY_H
#include <memory>
#include <mutex>

#include "concurrent_map.h"
#include "visibility.h"
namespace OHOS::AppDistributedKv {
class API_EXPORT CommunicationStrategy {
public:
    enum class Strategy : int32_t {
        // If AP is available, the AP is preferred. When AP is not available, only BR can be used; p2p is not support
        DEFAULT,
        // If AP is available, the AP is preferred. When AP is not available, BR is used for a small amount of data
        // and P2P is used for a large amount of data; The strategy takes effect only at the device online stage;
        ON_LINE_SELECT_CHANNEL,
        BUTT
    };
    static CommunicationStrategy &GetInstance();
    using Strategy = CommunicationStrategy::Strategy;
    void RegGetSyncDataSize(const std::string &type, const std::function<size_t(const std::string &)> &getDataSize);
    CommunicationStrategy::Strategy GetStrategy(const std::string &deviceId);
    void SetStrategy(const std::string &deviceId, Strategy strategy);
    void RemoveStrategy(const std::string &deviceId);
private:
    CommunicationStrategy() = default;
    ~CommunicationStrategy() = default;
    CommunicationStrategy(CommunicationStrategy const &) = delete;
    void operator=(CommunicationStrategy const &) = delete;
    CommunicationStrategy(CommunicationStrategy &&) = delete;
    CommunicationStrategy &operator=(CommunicationStrategy &&) = delete;

    size_t CalcSyncDataSize(const std::string &deviceId);

    static constexpr uint32_t SWITCH_CONNECTION_THRESHOLD = 75 * 1024u;
    ConcurrentMap<std::string, std::function<uint32_t(const std::string &)>> calcDataSizes_;
    ConcurrentMap<std::string, Strategy> strategys_;
};
}

#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_COMMUNICATION_STRATEGY_H