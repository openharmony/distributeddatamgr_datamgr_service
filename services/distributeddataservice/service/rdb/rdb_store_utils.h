/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RELATIONAL_STORE_UTILS_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RELATIONAL_STORE_UTILS_H
#include <cstdint>

namespace OHOS::DistributedRdb {
class RdbStoreUtils {
public:
    static int32_t ConvertNativeRdbStatus(int32_t status);
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RELATIONAL_STORE_UTILS_H