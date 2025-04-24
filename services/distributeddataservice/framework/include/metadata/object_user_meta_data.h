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

#ifndef DISTRIBUTEDDATAMGR_OBJECT_USER_META_DATA_H
#define DISTRIBUTEDDATAMGR_OBJECT_USER_META_DATA_H

#include "serializable/serializable.h"
namespace OHOS::DistributedData {
class API_EXPORT ObjectUserMetaData final : public Serializable {
public:
    std::string userId;

    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
    API_EXPORT ObjectUserMetaData();

    API_EXPORT static std::string GetKey();

private:
    static constexpr const char *KEY_PREFIX = "ObjectUserMetaData";
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_OBJECT_USER_META_DATA_H