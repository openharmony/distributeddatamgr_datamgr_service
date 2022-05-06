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
#include "iconnection.h"

#include "runtime_context.h"

namespace DistributedDB {
IConnection::IConnection()
    : connectionId_(0)
{
}

uint64_t IConnection::GetConnectionId()
{
    if (connectionId_ != 0) {
        return connectionId_;
    }
    std::lock_guard<std::mutex> autoLock(connectionIdLock_);
    // check again here, may be generated after get lock
    if (connectionId_ == 0) {
        connectionId_ = static_cast<uint64_t>(RuntimeContext::GetInstance()->GenerateSessionId());
    }
    return connectionId_;
}
}