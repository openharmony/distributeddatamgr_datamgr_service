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

#include "connect.h"
#include "connect_wrapper.h"

using ConnectInner::ConnectServiceInner;

#define API_EXPORT __attribute__((visibility ("default")))

#ifdef __cplusplus
extern "C" {
#endif
API_EXPORT OHOS::IRemoteObject *ConnectService(int userId)
{
    OHOS::sptr<OHOS::IRemoteObject> iRemoteObject = ConnectServiceInner(userId);
    if (iRemoteObject == nullptr) {
        return nullptr;
    }
    return iRemoteObject.GetRefPtr();
}
#ifdef __cplusplus
};
#endif