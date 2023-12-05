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

#include "snapshot/machine_status.h"


#ifndef DISTRIBUTEDDATAMGR_OBJECT_ASSET_MACHINE_H
#define DISTRIBUTEDDATAMGR_OBJECT_ASSET_MACHINE_H

namespace OHOS {
namespace DistributedObject {
using namespace OHOS::DistributedData;

typedef int32_t (*Action)(int32_t evtId, void* param, void* param2);

struct DFAAction {
    int32_t next;
    Action before;
    Action after;
};

class ObjectAssetMachine {
public:
    ObjectAssetMachine();

    static int32_t DFAPostEvent(AssetEvent eventId, TransferStatus& status, void* param, void* param2);

private:
};

} // namespace DistributedObject
} // namespace OHOS

#endif //DISTRIBUTEDDATAMGR_OBJECT_ASSET_MACHINE_H
