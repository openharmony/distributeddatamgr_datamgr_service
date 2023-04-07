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

#ifndef DISTRIBUTED_RDB_IRDB_RESULT_SET_H
#define DISTRIBUTED_RDB_IRDB_RESULT_SET_H

#include "iremote_broker.h"
#include "result_set.h"

namespace OHOS::DistributedRdb {
class IRdbResultSet : public NativeRdb::ResultSet, public IRemoteBroker {
public:
    using ColumnType = NativeRdb::ColumnType;
    virtual ~IRdbResultSet() = default;
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS::NativeRdb.IResultSet");
};
} // namespace OHOS::DistributedRdb
#endif // DISTRIBUTED_RDB_IRDB_RESULT_SET_H
