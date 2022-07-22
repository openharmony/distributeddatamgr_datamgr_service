/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <string>
#include <iostream>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "distributed_kv_data_manager.h"
#include "types.h"
#include "store_util.h"
#include "distributed_agent.h"
#include "refbase.h"
#include "hilog/log.h"
#include "directory_ex.h"

using namespace testing;
using namespace OHOS;
using namespace OHOS::DistributeSystemTest;
using namespace OHOS::DistributedKv;
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = {LOG_CORE, 0, "DistributedTestAgent"};
constexpr HiLogLabel LABEL_TEST = {LOG_CORE, 0, "KvStoreSyncCallbackTestImpl"};

class KvStoreSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    void SyncCompleted(const std::map<std::string, Status> &results);
    std::condition_variable cv_;
    bool compeleted_ = false;
    std::mutex mtx_;
};

void KvStoreSyncCallbackTestImpl::SyncCompleted(const std::map<std::string, Status> &results)
{
    for (const auto &result : results) {
        HiLog::Info(LABEL_TEST, "uuid = %{public}s, status = %{public}d",
                    StoreUtil::Anonymous(result.first).c_str(), result.second);
    }
    std::lock_guard<std::mutex> lck(mtx_);
    compeleted_ = true;
    cv_.notify_one();
}

class DistributedTestAgent : public DistributedAgent {
public:
    DistributedTestAgent();
    ~DistributedTestAgent();
    virtual bool SetUp();
    virtual bool TearDown();
    virtual int OnProcessMsg(const std::string &msg, int len, std::string &ret, int retLen);
    virtual int OnProcessCmd(const std::string &command, int len, const std::string &args,
        int argsLen, const std::string &expectValue, int expectValueLen);
    int Get(const std::string &msg, std::string &ret);
    int Put(const std::string &args);
    int Delete(const std::string &args);
    int Sync(const std::string &args);
    int RemoveDeviceData(const std::string &args);
    int ProcessMsg(const std::string &msg, std::string &ret);
    int ProcessCmd(const std::string &command, const std::string &args);
    static std::shared_ptr<SingleKvStore> singleKvStore_; // declare kvstore instance
    static constexpr int WAIT_FOR_TIME = 3; // wait for synccallback time is 3s.
    static constexpr int FILE_PERMISSION = 0777; // 0777 is to modify the permissions of the file.

private:
    DistributedKvDataManager manager_;
    std::vector<DeviceInfo> deviceInfos_;
    using CmdFunc = int (DistributedTestAgent::*)(const std::string &);
    std::map<std::string, CmdFunc> cmdFunMap_;
    using MsgFunc = int (DistributedTestAgent::*)(const std::string &, std::string &);
    std::map<std::string, MsgFunc> msgFunMap_;
};

std::shared_ptr<SingleKvStore> DistributedTestAgent::singleKvStore_ = nullptr;

DistributedTestAgent::DistributedTestAgent()
{}

DistributedTestAgent::~DistributedTestAgent()
{}

bool DistributedTestAgent::SetUp()
{
    msgFunMap_["get"] = &DistributedTestAgent::Get;
    cmdFunMap_["put"] = &DistributedTestAgent::Put;
    cmdFunMap_["delete"] = &DistributedTestAgent::Delete;
    cmdFunMap_["sync"] = &DistributedTestAgent::Sync;
    cmdFunMap_["removedevicedata"] = &DistributedTestAgent::RemoveDeviceData;

    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = false,
                        .kvStoreType = KvStoreType::SINGLE_VERSION };
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "student_single" }; // define kvstore(database) name.
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    manager_.GetSingleKvStore(options, appId, storeId, singleKvStore_);
    manager_.GetDeviceList(deviceInfos_, DeviceFilterStrategy::NO_FILTER);
    OHOS::ChangeModeDirectory(options.baseDir, FILE_PERMISSION);
    return true;
}

bool DistributedTestAgent::TearDown()
{
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
    return true;
}

int DistributedTestAgent::OnProcessCmd(const std::string &command, int len,
    const std::string &args, int argsLen, const std::string &expectValue, int expectValueLen)
{
    return DistributedTestAgent::ProcessCmd(command, args);
}

int DistributedTestAgent::OnProcessMsg(const std::string &msg, int len,
    std::string &ret, int retLen)
{
    return DistributedTestAgent::ProcessMsg(msg, ret);
}

int DistributedTestAgent::Get(const std::string &msg, std::string &ret)
{
    if (!singleKvStore_) {
        HiLog::Error(LABEL, "agent ERROR.");
        return Status::INVALID_ARGUMENT;
    }
    int index = msg.find(",");
    std::string skey = msg.substr(index+1);
    Key key(skey);
    Value value;
    auto status = singleKvStore_->Get(key, value);
    HiLog::Info(LABEL, "value = %{public}s", value.ToString().c_str());
    ret = value.ToString();
    ret += ",";
    ret += std::to_string(status);
    HiLog::Info(LABEL, "ret = %{public}s", ret.c_str());
    return ret.size();
}

int DistributedTestAgent::Put(const std::string &args)
{
    if (!singleKvStore_) {
        HiLog::Error(LABEL, "agent ERROR.");
        return Status::INVALID_ARGUMENT;
    }
    int index = args.find(",");
    std::string skey = args.substr(0, index);
    std::string sval = args.substr(index + 1);
    Key key(skey);
    Value value(sval);
    auto status = singleKvStore_->Put(key, value);
    return status;
}

int DistributedTestAgent::Delete(const std::string &args)
{
    if (!singleKvStore_) {
        HiLog::Error(LABEL, "agent ERROR.");
        return Status::INVALID_ARGUMENT;
    }
    Key key(args);
    Status status = singleKvStore_->Delete(key);
    return status;
}

int DistributedTestAgent::RemoveDeviceData(const std::string &args)
{
    if (!singleKvStore_) {
        HiLog::Error(LABEL, "agent ERROR.");
        return Status::INVALID_ARGUMENT;
    }
    HiLog::Info(LABEL, "deviceId = %{public}s", StoreUtil::Anonymous(deviceInfos_[0].deviceId).c_str());
    auto status = singleKvStore_->RemoveDeviceData(deviceInfos_[0].deviceId);
    return status;
}

int DistributedTestAgent::Sync(const std::string &args)
{
    if (!singleKvStore_) {
        HiLog::Error(LABEL, "agent ERROR.");
        return Status::INVALID_ARGUMENT;
    }
    auto syncCallback = std::make_shared<KvStoreSyncCallbackTestImpl>();
    auto status = singleKvStore_->RegisterSyncCallback(syncCallback);
    if (status != Status::SUCCESS) {
        HiLog::Info(LABEL, "register sync callback failed.");
        return Status::INVALID_ARGUMENT;
    }
    std::vector<std::string> deviceIds;
    for (const auto &device : deviceInfos_) {
        deviceIds.push_back(device.deviceId);
    }
    int32_t delay = std::stoi(args);
    HiLog::Info(LABEL, "delay = %{public}d", delay);
    std::unique_lock<std::mutex> lck(syncCallback->mtx_);
    status = singleKvStore_->Sync(deviceIds, SyncMode::PULL, delay);
    if (!syncCallback->cv_.wait_for(lck, std::chrono::seconds(WAIT_FOR_TIME),
        [&syncCallback]() { return syncCallback->compeleted_; })) {
        status = Status::TIME_OUT;
    }
    HiLog::Info(LABEL, "status = %{public}d", status);
    return status;
}

int DistributedTestAgent::ProcessMsg(const std::string &msg, std::string &ret)
{
    int index = msg.find(",");
    std::string argsMsg = msg.substr(0, index);
    std::map<std::string, MsgFunc>::iterator it = msgFunMap_.find(argsMsg);
    if (it != msgFunMap_.end()) {
        MsgFunc MsgFunc = msgFunMap_[argsMsg];
        return (this->*MsgFunc)(msg, ret);
    }
    return Status::INVALID_ARGUMENT;
}

int DistributedTestAgent::ProcessCmd(const std::string &command, const std::string &args)
{
    std::map<std::string, CmdFunc>::iterator it = cmdFunMap_.find(command);
    if (it != cmdFunMap_.end()) {
        CmdFunc CmdFunc = cmdFunMap_[command];
        return (this->*CmdFunc)(args);
    }
    return Status::INVALID_ARGUMENT;
}
}

int main()
{
    DistributedTestAgent obj;
    if (obj.SetUp()) {
        obj.Start("agent.desc");
        obj.Join();
    } else {
        HiLog::Error(LABEL, "Init environment failed.");
    }
    if (obj.TearDown()) {
        return 0;
    } else {
        HiLog::Error(LABEL, "Clear environment failed.");
        return -1;
    }
}
