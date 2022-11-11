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
 
#define LOG_TAG "CommunicationStrategyImpl"
#include "communication_strategy_impl.h"
#include "log_print.h"
#include "kvstore_utils.h"

namespace OHOS {
namespace AppDistributedKv {
using KvStoreUtils = OHOS::DistributedKv::KvStoreUtils;
void CommunicationStrategyImpl::RegObject(std::shared_ptr<CalcSyncDataSize> object)
{
    calcSyncDataSize_ = object;
}

CommunicationStrategy::Strategy CommunicationStrategyImpl::GetStrategy(const std::string &deviceId)
{
    if (calcSyncDataSize_ == nullptr || calcSyncDataSize_->CalcDataSize(deviceId) < SWITCH_CONNECTION_THRESHOLD) {
        return CommunicationStrategy::Strategy::DEFAULT;
    }
    return CommunicationStrategy::Strategy::ON_LINE_SELECT_CHANNEL;
}
}  // namespace AppDistributedKv
}  // namespace OHOS