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

#ifndef DISTRIBUTEDDATAFWK_RDB_TYPES_H
#define DISTRIBUTEDDATAFWK_RDB_TYPES_H

#include <functional>
#include <map>
#include <string>

namespace OHOS::DistributedRdb {
enum RdbDistributedType {
    RDB_DEVICE_COLLABORATION,
    RDB_DISTRIBUTED_TYPE_MAX
};

struct RdbSyncerParam {
    std::string bundleName_;
    std::string path_;
    std::string storeName_;
    int type_ = RDB_DEVICE_COLLABORATION;
    bool isAutoSync_ = false;
};

enum SyncMode {
    PUSH,
    PULL,
};

struct SyncOption {
    SyncMode mode;
    bool isBlock;
};

using SyncResult = std::map<std::string, int>; // networkId
using SyncCallback = std::function<void(SyncResult&)>;

enum SubscribeMode {
    LOCAL,
    REMOTE,
    LOCAL_AND_REMOTE,
};

struct SubscribeOption {
    SubscribeMode mode;
};

struct DropOption {
};
}
#endif
