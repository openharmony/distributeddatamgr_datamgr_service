/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "dumphelper_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "accesstoken_kit.h"
#include "dump_helper.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"
#include "token_setproc.h"
#include <vector>

using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;

namespace OHOS {

bool DumpFuzz(const uint8_t *data, size_t size)
{
    int connId = static_cast<int>(*data);
    std::vector<std::string> args;
    const std::string argstest1 ="OHOS.DistributedData.DumpHelper1";
    const std::string argstest2 ="OHOS.DistributedData.DumpHelper2";
    args.emplace_back(argstest1);
    args.emplace_back(argstest2);
    DumpHelper::GetInstance().Dump(connId, args);

    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    OHOS::DumpFuzz(data, size);

    return 0;
}