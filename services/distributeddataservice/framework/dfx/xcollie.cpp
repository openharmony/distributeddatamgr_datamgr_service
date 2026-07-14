/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "dfx/xcollie.h"

#include <utility>

namespace OHOS::DistributedData {
XCollieDelegate *XCollieDelegate::instance_ = nullptr;

bool XCollieDelegate::RegisterXCollieInstance(XCollieDelegate *instance)
{
    if (instance_ != nullptr) {
        return false;
    }
    instance_ = instance;
    return true;
}

XCollieDelegate *XCollieDelegate::GetInstance()
{
    return instance_;
}

XCollie::XCollie(const std::string &tag, uint32_t flag, uint32_t timeoutSeconds, std::function<void(void *)> func,
                 void *arg)
{
    auto *instance = XCollieDelegate::GetInstance();
    if (instance != nullptr) {
        id_ = instance->SetTimer(tag, timeoutSeconds, std::move(func), arg, flag);
    }
}

XCollie::~XCollie()
{
    auto *instance = XCollieDelegate::GetInstance();
    if (instance != nullptr && id_ != -1) {
        instance->CancelTimer(id_);
    }
}
} // namespace OHOS::DistributedData
