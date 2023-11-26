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

#include "cloud/asset_loader.h"
namespace OHOS::DistributedData {
int32_t AssetLoader::Download(const std::string &tableName, const std::string &gid, const Value &prefix,
    VBucket &assets)
{
    return E_NOT_SUPPORT;
}
int32_t AssetLoader::RemoveLocalAssets(const std::string &tableName, const std::string &gid,
    const Value &prefix, VBucket &assets)
{
    return E_NOT_SUPPORT;
}
} // namespace OHOS::DistributedData
