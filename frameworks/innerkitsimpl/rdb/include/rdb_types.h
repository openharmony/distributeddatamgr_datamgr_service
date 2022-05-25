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
#include <vector>

namespace OHOS::DistributedRdb {
enum RdbStatus {
    RDB_OK,
    RDB_ERROR,
};

enum RdbDistributedType {
    RDB_DEVICE_COLLABORATION = 10,
    RDB_DISTRIBUTED_TYPE_MAX
};

struct RdbSyncerParam {
    std::string bundleName_;
    std::string relativePath_;
    std::string storeName_;
    std::string encryptLevel_;
    std::string realPath_;
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
using SyncCallback = std::function<void(const SyncResult&)>;

enum RdbPredicateOperator {
    EQUAL_TO,
    NOT_EQUAL_TO,
    AND,
    OR,
    ORDER_BY,
    LIMIT,
    OPERATOR_MAX
};

struct RdbPredicateOperation {
    RdbPredicateOperator operator_;
    std::string field_;
    std::vector<std::string> values_;
};

struct RdbPredicates {
    inline void AddOperation(const RdbPredicateOperator op, const std::string& field,
                             const std::string& value)
    {
        operations_.push_back({ op, field, { value } });
    }
    inline void AddOperation(const RdbPredicateOperator op, const std::string& field,
                             const std::vector<std::string>& values)
    {
        operations_.push_back({ op, field, values });
    }

    std::string table_;
    std::vector<std::string> devices_;
    std::vector<RdbPredicateOperation> operations_;
};

enum SubscribeMode {
    REMOTE,
    SUBSCRIBE_MODE_MAX
};

struct SubscribeOption {
    SubscribeMode mode;
};

class RdbStoreObserver {
public:
    virtual void OnChange(const std::vector<std::string>& devices) = 0; // networkid
};

struct DropOption {
};
}
#endif
