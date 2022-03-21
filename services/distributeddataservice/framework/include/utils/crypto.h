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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_UTILS_CRYPTO_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_UTILS_CRYPTO_H
#include <string>
#include <vector>
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class Crypto {
public:
    API_EXPORT static std::string Sha256(const std::string &text, bool isUpper = false);
    API_EXPORT static std::string Sha256(const void *data, size_t size, bool isUpper = false);
    API_EXPORT static std::vector<uint8_t> Random(int32_t len);
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_UTILS_CRYPTO_H
