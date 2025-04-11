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

#ifndef DISTRIBUTEDDATAMGR_AUTH_DELEGATE_H
#define DISTRIBUTEDDATAMGR_AUTH_DELEGATE_H

#include <string>

#include "metadata/user_meta_data.h"
#include "serializable/serializable.h"
#include "commu_types.h"
namespace OHOS::DistributedData {
using AclParams = OHOS::AppDistributedKv::AclParams;
enum AUTH_GROUP_TYPE {
    ALL_GROUP = 0,
    IDENTICAL_ACCOUNT_GROUP = 1,
    PEER_TO_PEER_GROUP = 256,
    COMPATIBLE_GROUP = 512,
    ACROSS_ACCOUNT_AUTHORIZE_GROUP = 1282
};

class AuthHandler {
public:
    virtual std::pair<bool, bool> CheckAccess(int localUserId, int peerUserId,
        const std::string &peerDeviceId, const AclParams &aclParams, const std::string &appId);
};

class AuthDelegate {
public:
    API_EXPORT static AuthHandler *GetInstance();
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_AUTH_DELEGATE_H
