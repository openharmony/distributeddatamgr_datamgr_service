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

#include "iprocesscommunicator_fuzzer.h"
#include <list>
#include "iprocess_communicator.h"
#include "distributeddb_tools_test.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

class IProcessCommunicatorFuzzer {
    /* Keep C++ file names the same as the class name */
};

namespace OHOS {
class ProcessCommunicatorFuzzTest : public DistributedDB::IProcessCommunicator {
public:
    ProcessCommunicatorFuzzTest() {}
    ~ProcessCommunicatorFuzzTest() {}
    DistributedDB::DBStatus Start(const std::string &processLabel) override
    {
        return DistributedDB::OK;
    }
    // The Stop should only be called after Start successfully
    DistributedDB::DBStatus Stop() override
    {
        return DistributedDB::OK;
    }
    DistributedDB::DBStatus RegOnDeviceChange(const DistributedDB::OnDeviceChange &callback) override
    {
        onDeviceChange_ = callback;
        return DistributedDB::OK;
    }
    DistributedDB::DBStatus RegOnDataReceive(const DistributedDB::OnDataReceive &callback) override
    {
        onDataReceive_ = callback;
        return DistributedDB::OK;
    }
    DistributedDB::DBStatus SendData(const DistributedDB::DeviceInfos &dstDevInfo,
        const uint8_t *datas, uint32_t length) override
    {
        return DistributedDB::OK;
    }
    uint32_t GetMtuSize() override
    {
        return 1 * 1024  * 1024; // 1 * 1024 * 1024 Byte.
    }
    DistributedDB::DeviceInfos GetLocalDeviceInfos() override
    {
        DistributedDB::DeviceInfos info;
        info.identifier = "default";
        return info;
    }

    std::vector<DistributedDB::DeviceInfos> GetRemoteOnlineDeviceInfosList() override
    {
        std::vector<DistributedDB::DeviceInfos> info;
        return info;
    }

    bool IsSameProcessLabelStartedOnPeerDevice(const DistributedDB::DeviceInfos &peerDevInfo) override
    {
        return true;
    }

    void FuzzOnDeviceChange(const DistributedDB::DeviceInfos &devInfo, bool isOnline)
    {
        if (onDeviceChange_ == nullptr) {
            return;
        }
        onDeviceChange_(devInfo, isOnline);
    }

    void FuzzOnDataReceive(const  DistributedDB::DeviceInfos &devInfo, const uint8_t* data, size_t size)
    {
        if (onDataReceive_ == nullptr) {
            return;
        }
        onDataReceive_(devInfo, data, size);
    }

private:
    DistributedDB::OnDeviceChange onDeviceChange_ = nullptr;
    DistributedDB::OnDataReceive onDataReceive_ = nullptr;
};

void CommunicatorFuzzer(const uint8_t* data, size_t size)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    std::string rawString(reinterpret_cast<const char*>(data), size);
    KvStoreDelegateManager::SetProcessLabel(rawString, "defaut");
    auto communicator = std::make_shared<ProcessCommunicatorFuzzTest>();
    KvStoreDelegateManager::SetProcessCommunicator(communicator);
    std::string testDir;
    DistributedDBToolsTest::TestDirInit(testDir);
    KvStoreConfig config;
    config.dataDir = testDir;
    kvManager.SetKvStoreConfig(config);
    KvStoreNbDelegate::Option option = {true, false, false};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    kvManager.GetKvStore(rawString, option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    DeviceInfos device = {"defaut"};
    communicator->FuzzOnDataReceive(device, data, size);
    if (kvNbDelegatePtr != nullptr) {
        kvManager.CloseKvStore(kvNbDelegatePtr);
        kvManager.DeleteKvStore(rawString);
    }
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // 4 bytes is required
    if (size < 4) {
        return 0;
    }
    OHOS::CommunicatorFuzzer(data, size);
    return 0;
}
