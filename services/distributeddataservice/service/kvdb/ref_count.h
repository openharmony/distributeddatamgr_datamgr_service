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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_REF_COUNT_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_REF_COUNT_H
#include <atomic>
#include <functional>
#include <memory>
namespace OHOS::DistributedKv {
class RefCount final {
public:
    RefCount();
    explicit RefCount(std::function<void()> action);
    RefCount(const RefCount &other);
    RefCount(RefCount &&other) noexcept;
    RefCount &operator=(const RefCount &other);
    RefCount &operator=(RefCount &&other) noexcept;
    operator bool() const;

private:
    std::shared_ptr<const char> ref_;
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_REF_COUNT_H
