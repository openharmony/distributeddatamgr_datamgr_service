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

#ifndef DISTRIBUTEDDATAMGR_SESSIONMANAGER_H
#define DISTRIBUTEDDATAMGR_SESSIONMANAGER_H

#include <string>
#include <vector>

#include "serializable/serializable.h"

namespace OHOS::DistributedData {
struct SessionPoint {
    std::string deviceId;
    uint32_t userId;
    std::string appId;
    std::string storeId;
};

class Session : public Serializable {
public:
    std::string sourceDeviceId;
    std::string targetDeviceId;
    uint32_t sourceUserId;
    std::vector<uint32_t> targetUserIds;
    std::string appId;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    inline bool IsValid()
    {
        return !targetUserIds.empty();
    }
};

class SessionManager {
public:
    static SessionManager &GetInstance();
    Session GetSession(const SessionPoint &from, const std::string &targetDeviceId) const;
    bool CheckSession(const SessionPoint &from, const SessionPoint &to) const;
private:
    bool GetAuthParams(const SessionPoint &from, std::string &bundleName, int32_t &auth) const;
};
} // namespace OHOS::DistributedData

#endif // DISTRIBUTEDDATAMGR_SESSIONMANAGER_H
