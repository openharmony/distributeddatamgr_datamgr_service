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

#include "distributed_test_helper.h"

using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributeSystemTest;
using namespace OHOS::HiviewDFX;

void DistributedTestHelper::SetUpTestCase(void)
{}

void DistributedTestHelper::TearDownTestCase(void)
{}

void DistributedTestHelper::SetUp()
{}

void DistributedTestHelper::TearDown()
{}

void DistributedTestHelper::TestBody()
{}

Status DistributedTestHelper::GetRemote(const Key &key, Value &value)
{
    std::string msg = "get";
    msg += ",";
    msg += key.ToString();
    Status status;
    SendMessage(AGENT_NO::ONE, msg, msg.size(), [&](const std::string &buf, int len)->bool {
        int index = buf.find(",");
        std::string temp = buf.substr(0, index);
        value = temp;
        temp = buf.substr(index + 1);
        status = Status(std::stoi(temp));
        return true;
    });
    return status;
}

Status DistributedTestHelper::PutRemote(const Key &key, const Value &value)
{
    std::string command = "put";
    std::string args = key.ToString();
    args += ",";
    args += value.ToString();
    RunCmdOnAgent(AGENT_NO::ONE, command, args, "0");
    Status status = Status(GetReturnVal());
    return status;
}

Status DistributedTestHelper::DeleteRemote(const Key &key)
{
    std::string command = "delete";
    std::string skey = key.ToString();
    RunCmdOnAgent(AGENT_NO::ONE, command, skey, "0");
    Status status = Status(GetReturnVal());
    return status;
}

Status DistributedTestHelper::RemoveDeviceDataRemote()
{
    std::string command = "removedevicedata";
    RunCmdOnAgent(AGENT_NO::ONE, command, "0", "0");
    Status status = Status(GetReturnVal());
    return status;
}

Status DistributedTestHelper::SyncRemote(SyncMode mode, uint32_t delay)
{
    std::string command = "sync";
    std::string sdelay = std::to_string(delay);
    RunCmdOnAgent(AGENT_NO::ONE, command, sdelay, "0");
    Status status = Status(GetReturnVal());
    return status;
}