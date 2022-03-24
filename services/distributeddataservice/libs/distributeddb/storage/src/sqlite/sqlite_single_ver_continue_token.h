/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef SQLITE_SINGLE_VER_CONTINUE_TOKEN_H
#define SQLITE_SINGLE_VER_CONTINUE_TOKEN_H

#include <map>

#include "db_types.h"
#include "query_object.h"
#include "single_ver_kvdb_sync_interface.h"

namespace DistributedDB {
class SQLiteSingleVerContinueToken {
public:
    // For one device.
    SQLiteSingleVerContinueToken(Timestamp begin, Timestamp end);

    // For one device in query sync.
    SQLiteSingleVerContinueToken(const SyncTimeRange &timeRange, const QueryObject &queryObject);

    // For multiple device.
    explicit SQLiteSingleVerContinueToken(MulDevTimeRanges timeRanges);

    ~SQLiteSingleVerContinueToken();

    /*
     * function: Check the magic number at the beginning and end of the SingleVerContinueToken.
     * returnValue: Return true if the begin and end magic number is OK.
     *              Return false if the begin or end magic number is error.
     */
    bool CheckValid() const;

    Timestamp GetQueryBeginTime() const;
    Timestamp GetQueryEndTime() const;
    Timestamp GetDeletedBeginTime() const;
    Timestamp GetDeletedEndTime() const;

    void SetNextBeginTime(const DeviceID &deviceID, Timestamp nextBeginTime);
    const MulDevTimeRanges &GetTimeRanges();
    void SetDeletedNextBeginTime(const DeviceID &deviceID, Timestamp nextBeginTime);
    const MulDevTimeRanges &GetDeletedTimeRanges() const;

    void FinishGetQueryData();
    void FinishGetDeletedData();

    bool IsGetQueryDataFinished() const;
    bool IsGetDeletedDataFinished() const;

    bool IsQuerySync() const;
    QueryObject GetQuery() const;

private:
    void RemovePrevDevAndSetBeginTime(const DeviceID &deviceID, Timestamp nextBeginTime, MulDevTimeRanges &timeRanges);

    Timestamp GetBeginTimestamp(const MulDevTimeRanges &timeRanges) const;
    Timestamp GetEndTimestamp(const MulDevTimeRanges &timeRanges) const;

    static const unsigned int MAGIC_BEGIN = 0x600D0AC7;  // for token guard
    static const unsigned int MAGIC_END = 0x0AC7600D;    // for token guard
    unsigned int magicBegin_ = MAGIC_BEGIN;
    std::map<DeviceID, QueryObject> queryObject_;
    MulDevTimeRanges timeRanges_;
    MulDevTimeRanges deleteTimeRanges_;
    unsigned int magicEnd_ = MAGIC_END;
};
}  // namespace DistributedDB
#endif  // SQLITE_SINGLE_VER_CONTINUE_TOKEN_H