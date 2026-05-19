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

#define LOG_TAG "BmsDelegate"
#include "bms/bms_delegate.h"

#include "log_print.h"

namespace OHOS::DistributedData {
std::mutex BmsDelegate::mutex_;
std::shared_ptr<BmsDelegate> BmsDelegate::instance_ = nullptr;

std::shared_ptr<BmsDelegate> BmsDelegate::GetInstance()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (instance_ != nullptr) {
        return instance_;
    }
    instance_ = std::make_shared<BmsDelegate>();
    ZLOGW("no register, new instance");
    return instance_;
}

bool BmsDelegate::RegisterInstance(std::shared_ptr<BmsDelegate> instance)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    instance_ = std::move(instance);
    return true;
}

std::string BmsDelegate::GetCallerAppIdentifier(const std::string &bundleName, int32_t userId)
{
    return "";
}
} // namespace OHOS::DistributedData
