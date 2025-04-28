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

#ifndef OHOS_DISTRIBUTEDDATA_AUTH_DELEGATE_MOCK_H
#define OHOS_DISTRIBUTEDDATA_AUTH_DELEGATE_MOCK_H

#include <gmock/gmock.h>

#include <vector>

#include "auth_delegate.h"
namespace OHOS {
namespace DistributedData {
class AuthHandlerMock : public AuthHandler {
public:
    static AuthHandlerMock &GetInstance()
    {
        static AuthHandlerMock instance;
        return instance;
    }
    MOCK_METHOD((std::pair<bool, bool>), CheckAccess,
        (int localUserId, int peerUserId, const std::string &peerDeviceId, const AclParams &aclParams), (override));
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTEDDATA_AUTH_DELEGATE_MOCK_H