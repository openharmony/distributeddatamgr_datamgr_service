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
#ifndef OHOS_DISTRIBUTED_FILE_DAEMON_MANAGER_MOCK_H
#define OHOS_DISTRIBUTED_FILE_DAEMON_MANAGER_MOCK_H

#include <gmock/gmock.h>

#include "distributed_file_daemon_manager.h"
namespace OHOS {
namespace Storage {
namespace DistributedFile {
class BDistributedFileDaemonManager {
public:
    virtual int32_t RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) = 0;
    virtual int32_t UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) = 0;
    BDistributedFileDaemonManager() = default;
    virtual ~BDistributedFileDaemonManager() = default;

private:
    static inline std::shared_ptr<BDistributedFileDaemonManager> fileDaemonManger_ = nullptr;
};

class DistributedFileDaemonManagerMock : public BDistributedFileDaemonManager {
public:
    MOCK_METHOD(int32_t, RegisterAssetCallback, (const sptr<IAssetRecvCallback> &recvCallback));
    MOCK_METHOD(int32_t, UnRegisterAssetCallback, (const sptr<IAssetRecvCallback> &recvCallback));
};

class DistributedFileDaemonManagerImpl : public DistributedFileDaemonManager {
public:
    static DistributedFileDaemonManagerImpl &GetInstance()
    {
        static DistributedFileDaemonManagerImpl instance;
        return instance;
    }
    int32_t RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) override;
    int32_t UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) override;

    int32_t OpenP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) override
    {
        return 0;
    }
    int32_t CloseP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) override
    {
        return 0;
    }
    int32_t OpenP2PConnectionEx(const std::string &networkId, sptr<IFileDfsListener> remoteReverseObj) override
    {
        return 0;
    }
    int32_t CloseP2PConnectionEx(const std::string &networkId) override
    {
        return 0;
    }
    int32_t PrepareSession(const std::string &srcUri, const std::string &dstUri, const std::string &srcDeviceId,
        const sptr<IRemoteObject> &listener, HmdfsInfo &info) override
    {
        return 0;
    }
    int32_t CancelCopyTask(const std::string &sessionName) override
    {
        return 0;
    }
    int32_t CancelCopyTask(const std::string &srcUri, const std::string &dstUri) override
    {
        return 0;
    }

    int32_t PushAsset(int32_t userId, const sptr<AssetObj> &assetObj,
        const sptr<IAssetSendCallback> &sendCallback) override
    {
        return 0;
    }
    int32_t GetSize(const std::string &uri, bool isSrcUri, uint64_t &size) override
    {
        return 0;
    }
    int32_t IsDirectory(const std::string &uri, bool isSrcUri, bool &isDirectory) override
    {
        return 0;
    }
    int32_t Copy(const std::string &srcUri, const std::string &destUri, ProcessCallback processCallback) override
    {
        return 0;
    }
    int32_t Cancel(const std::string &srcUri, const std::string &destUri) override
    {
        return 0;
    }
    int32_t Cancel() override
    {
        return 0;
    }
    int32_t GetDfsSwitchStatus(const std::string &networkId, int32_t &switchStatus) override
    {
        return 0;
    }
    int32_t UpdateDfsSwitchStatus(int32_t switchStatus) override
    {
        return 0;
    }
    int32_t GetConnectedDeviceList(std::vector<DfsDeviceInfo> &deviceList) override
    {
        return 0;
    }
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif