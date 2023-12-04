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
#ifndef DISTRIBUTEDDATAMGR_OBJECT_ASSET_LOADER_H
#define DISTRIBUTEDDATAMGR_OBJECT_ASSET_LOADER_H

#include <string>
#include "object_types.h"

namespace OHOS {
namespace DistributedObject {
class ObjectAssetLoader {
public:
    static ObjectAssetLoader *GetInstance();
    bool DownLoad(const int32_t userId, const std::string &bundleName,
        const std::string &deviceId, const ObjectStore::Asset &assetValue);

private:
    ObjectAssetLoader() = default;
    ~ObjectAssetLoader() = default;
    ObjectAssetLoader(const ObjectAssetLoader &) = delete;
    ObjectAssetLoader &operator=(const ObjectAssetLoader &) = delete;

    static constexpr int WAIT_TIME = 60;
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_ASSET_LOADER_H