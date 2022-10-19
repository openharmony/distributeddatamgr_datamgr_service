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

#include "ref_count.h"
namespace OHOS::DistributedKv {
RefCount::RefCount()
{
}

RefCount::RefCount(std::function<void()> action)
{
    ref_ = std::shared_ptr<const char>("RefCount", [action](const char *) {
        if (action) {
            action();
        }
    });
}

RefCount::RefCount(const RefCount &other)
{
    if (this == &other) {
        return ;
    }
    ref_ = other.ref_;
}

RefCount::RefCount(RefCount &&other) noexcept
{
    if (this == &other) {
        return ;
    }
    ref_ = std::move(other.ref_);
}

RefCount &RefCount::operator=(const RefCount &other)
{
    if (this == &other) {
        return *this;
    }
    ref_ = other.ref_;
    return *this;
}

RefCount &RefCount::operator=(RefCount &&other) noexcept
{
    if (this == &other) {
        return *this;
    }
    ref_ = std::move(other.ref_);
    return *this;
}

RefCount::operator bool() const
{
    return ref_ != nullptr;
}
} // namespace OHOS::DistributedKv