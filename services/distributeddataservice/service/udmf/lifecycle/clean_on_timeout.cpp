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

#include "clean_on_timeout.h"

namespace OHOS {
namespace UDMF {
Status CleanOnTimeout::OnStart(const std::string &intention)
{
    return LifeCyclePolicy::OnTimeout(intention);
}

Status CleanOnTimeout::OnGot(const UnifiedKey &key)
{
    return E_OK;
}
} // namespace UDMF
} // namespace OHOS
