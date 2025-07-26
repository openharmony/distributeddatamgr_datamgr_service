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

#include "distributed_file_daemon_manager_mock.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
constexpr const int32_t ERROR = -1;

DistributedFileDaemonManager &DistributedFileDaemonManager::GetInstance()
{
    return DistributedFileDaemonManagerImpl::GetInstance();
}

int32_t DistributedFileDaemonManagerImpl::RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
{
    if (BDistributedFileDaemonManager::fileDaemonManger_ == nullptr) {
        return ERROR;
    }
    return BDistributedFileDaemonManager::fileDaemonManger_->RegisterAssetCallback(recvCallback);
}

int32_t DistributedFileDaemonManagerImpl::UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
{
    if (BDistributedFileDaemonManager::fileDaemonManger_ == nullptr) {
        return ERROR;
    }
    return BDistributedFileDaemonManager::fileDaemonManger_->UnRegisterAssetCallback(recvCallback);
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS