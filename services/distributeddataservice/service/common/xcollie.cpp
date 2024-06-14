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
#include "xcollie.h"

namespace OHOS::DistributedData {
XCollie::XCollie(const std::string &tag, uint32_t flag, uint32_t timeoutSeconds,
    std::function<void(void *)> func, void *arg)
{
    id_ = HiviewDFX::XCollie::GetInstance().SetTimer(tag, timeoutSeconds, func, arg, flag);
}
XCollie::~XCollie()
{
    HiviewDFX::XCollie::GetInstance().CancelTimer(id_);
}
} // namespace OHOS::DistributedData