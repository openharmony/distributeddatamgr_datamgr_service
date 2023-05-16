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
#define LOG_TAG "DivStrategy"
#include "div_strategy.h"

#include "log_print.h"
namespace OHOS::DataShare {
bool DivStrategy::operator()(std::shared_ptr<Context> context)
{
    if (check_ == nullptr || trueAction_ == nullptr || falseAction_ == nullptr) {
        ZLOGE("wrong strategy");
        return false;
    }
    return (*check_)(context) ? (*trueAction_)(context) : (*falseAction_)(context);
}
} // namespace OHOS::DataShare