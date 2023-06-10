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
#define LOG_TAG "SeqStrategy"
#include "seq_strategy.h"

#include "log_print.h"
namespace OHOS::DataShare {
bool SeqStrategy::operator()(std::shared_ptr<Context> context)
{
    for (auto &action : actions_) {
        if (!(*action)(context)) {
            return false;
        }
    }
    return true;
}
bool SeqStrategy::Init(std::initializer_list<Strategy *> strategies)
{
    for (const auto &item: strategies) {
        if (item == nullptr) {
            return false;
        }
    }
    for (const auto &item: strategies) {
        actions_.emplace_back(item);
    }
    return true;
}
bool SeqStrategy::IsEmpty()
{
    return actions_.empty();
}
} // namespace OHOS::DataShare