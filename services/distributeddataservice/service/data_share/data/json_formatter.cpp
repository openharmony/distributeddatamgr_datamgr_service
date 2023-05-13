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
#define LOG_TAG "JsonFormatter"
#include "json_formatter.h"

#include "log_print.h"

namespace OHOS::DataShare {
bool JsonFormatter::Marshal(json &node) const
{
    if (value_ == nullptr) {
        ZLOGE("null value %{public}s", key_.c_str());
        return false;
    }
    return SetValue(node[key_], *value_);
}

bool JsonFormatter::Unmarshal(const json &node)
{
    if (value_ == nullptr) {
        ZLOGE("null value %{public}s", key_.c_str());
        return false;
    }
    return GetValue(node, key_, *value_);
}
} // namespace OHOS::DataShare