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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_ASSET_LOADER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_ASSET_LOADER_H
#include <vector>

#include "store/general_value.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT AssetLoader {
public:
    virtual ~AssetLoader() = default;
    virtual int32_t Download(
        const std::string &tableName, const std::string &gid, const Value &prefix, VBucket &assets);
    virtual int32_t RemoveLocalAssets(
        const std::string &tableName, const std::string &gid, const Value &prefix, VBucket &assets);
    virtual int32_t Cancel();
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_ASSET_LOADER_H