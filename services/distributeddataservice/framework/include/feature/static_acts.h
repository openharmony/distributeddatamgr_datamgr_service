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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORK_STATIC_ACTS_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORK_STATIC_ACTS_H
#include <cstdio>
#include <string>
#include "error/general_error.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT StaticActs {
public:
    virtual ~StaticActs();
    virtual int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index);
    virtual int32_t OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index);
    virtual int32_t OnAppInstall(const std::string &bundleName, int32_t user, int32_t index);
    virtual int32_t OnClearAppStorage(const std::string &bundleName, int32_t user, int32_t index, int32_t tokenId);
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORK_STATIC_ACTS_H
