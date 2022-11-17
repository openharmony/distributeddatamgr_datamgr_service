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
 
#define LOG_TAG "CommunicationStrategy"
#include "communication_strategy.h"
#include "log_print.h"
#include "kvstore_utils.h"

namespace OHOS {
namespace AppDistributedKv {
CommunicationStrategy &CommunicationStrategy::GetInstance()
{
    static CommunicationStrategy instance;
    return instance;
}

void CommunicationStrategy::RegGetSyncDataSize(const std::string &type,
    const std::function<size_t(const std::string &)> &getDataSize)
{
    calcDataSizes_.InsertOrAssign(type, getDataSize);
}

size_t CommunicationStrategy::CalcSyncDataSize(const std::string &deviceId)
{
    size_t dataSize = 0;
    calcDataSizes_.ForEach([&dataSize, &deviceId](const std::string &key, auto &value) {
        if (value) {
            dataSize += value(deviceId);
        }
        return false;
    });
    ZLOGD("calc data size:%{public}zu.", dataSize);
    return dataSize;
}

void CommunicationStrategy::SetStrategy(const std::string &deviceId, Strategy strategy,
                                        const std::function<void(const std::string &, Strategy)> &action)
{
    auto value = strategy;
    if (strategy == Strategy::ON_LINE_SELECT_CHANNEL && CalcSyncDataSize(deviceId) < SWITCH_CONNECTION_THRESHOLD) {
        value = Strategy::DEFAULT;
    }
    if (action) {
        action(deviceId, value);
    }
    strategys_.InsertOrAssign(deviceId, value);
    return ;
}

CommunicationStrategy::Strategy CommunicationStrategy::GetStrategy(const std::string &deviceId)
{
    auto result = strategys_.Find(deviceId);
    if (!result.first) {
        return Strategy::DEFAULT;
    }

    return result.second;
}
}  // namespace AppDistributedKv
}  // namespace OHOS