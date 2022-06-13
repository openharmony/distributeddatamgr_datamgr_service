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

#ifndef DISTRIBUTEDDATAMGR_USER_META_DATA_H
#define DISTRIBUTEDDATAMGR_USER_META_DATA_H
#include <string>
#include <vector>

#include "serializable/serializable.h"
namespace OHOS::DistributedData {
class API_EXPORT UserStatus final : public Serializable {
public:
    int32_t id;
    bool isActive;
    API_EXPORT UserStatus() = default;
    API_EXPORT ~UserStatus() = default;
    API_EXPORT UserStatus(int id, bool isActive);
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};

class API_EXPORT UserMetaData final : public Serializable {
public:
    std::string deviceId;
    std::vector<UserStatus> users;

    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};

class UserMetaRow {
public:
    API_EXPORT static constexpr const char *KEY_PREFIX = "UserMeta";
    API_EXPORT static std::string GetKeyFor(const std::string &key);
};
} // namespace OHOS::DistributedData

#endif // DISTRIBUTEDDATAMGR_USER_META_DATA_H
