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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SHARING_CENTER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SHARING_CENTER_H

#include <vector>
#include <string>
#include <tuple>
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT SharingCenter {
public:
    enum Role {
        ROLE_NIL = -1,
        ROLE_INVITER,
        ROLE_INVITEE,
        ROLE_BUTT
    };

    enum Confirmation {
        CFM_NIL = -1,
        CFM_UNKNOWN,
        CFM_ACCEPTED,
        CFM_REJECTED,
        CFM_SUSPENDED,
        CFM_UNAVAILABLE,
        CFM_BUTT
    };

    struct Privilege {
        bool writable = false;
        bool readable = false;
        bool creatable = false;
        bool deletable = false;
        bool shareable = false;
    };

    struct Participant {
        std::string identity;
        int32_t role = Role::ROLE_NIL;
        int32_t state = Confirmation::CFM_NIL;
        Privilege privilege;
        std::string attachInfo;
    };

    using Participants = std::vector<Participant>;
    using Results = std::tuple<int32_t, std::string, std::vector<std::pair<int32_t, std::string>>>;
    using QueryResults = std::tuple<int32_t, std::string, Participants>;

    enum SharingCode : int32_t {
        SUCCESS = 0,
        REPEATED_REQUEST,
        NOT_INVITER,
        NOT_INVITER_OR_INVITEE,
        OVER_QUOTA,
        TOO_MANY_PARTICIPANTS,
        INVALID_ARGS,
        NETWORK_ERROR,
        CLOUD_DISABLED,
        SERVER_ERROR,
        INNER_ERROR,
        INVALID_INVITATION,
        RATE_LIMIT,
        IPC_ERROR,
        CUSTOM_ERROR = 1000,
    };

    virtual Results Share(int32_t userId, const std::string &bundleName, const std::string &sharingRes,
        const Participants &participants);
    virtual Results Unshare(int32_t userId, const std::string &bundleName, const std::string &sharingRes,
        const Participants &participants);
    virtual std::pair<int32_t, std::string> Exit(int32_t userId, const std::string &bundleName,
        const std::string &sharingRes);
    virtual Results ChangePrivilege(int32_t userId, const std::string &bundleName, const std::string &sharingRes,
        const Participants &participants);
    virtual QueryResults Query(int32_t userId, const std::string &bundleName, const std::string &sharingRes);
    virtual QueryResults QueryByInvitation(int32_t userId, const std::string &bundleName,
        const std::string &invitation);
    virtual std::tuple<int32_t, std::string, std::string> ConfirmInvitation(int32_t userId,
        const std::string &bundleName, const std::string &invitation, int32_t confirmation);
    virtual std::pair<int32_t, std::string> ChangeConfirmation(int32_t userId,
        const std::string &bundleName, const std::string &sharingRes, int32_t confirmation);
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SHARING_CENTER_H
