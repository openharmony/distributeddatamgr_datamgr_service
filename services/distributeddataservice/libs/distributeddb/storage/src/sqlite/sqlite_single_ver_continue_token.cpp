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

#include "sqlite_single_ver_continue_token.h"

namespace DistributedDB {
SQLiteSingleVerContinueToken::SQLiteSingleVerContinueToken(TimeStamp begin, TimeStamp end)
    : timeRanges_(MulDevTimeRanges{{"", {begin, end}}})
{}

SQLiteSingleVerContinueToken::SQLiteSingleVerContinueToken(
    const SyncTimeRange &timeRange, const QueryObject &queryObject)
    : queryObject_(std::map<DeviceID, QueryObject>{{"", queryObject}}),
      timeRanges_(MulDevTimeRanges{{"", {timeRange.beginTime, timeRange.endTime}}}),
      deleteTimeRanges_(MulDevTimeRanges{{"", {timeRange.deleteBeginTime, timeRange.deleteEndTime}}})
{}

SQLiteSingleVerContinueToken::SQLiteSingleVerContinueToken(MulDevTimeRanges timeRanges)
    : timeRanges_(timeRanges)
{}

SQLiteSingleVerContinueToken::~SQLiteSingleVerContinueToken()
{}

bool SQLiteSingleVerContinueToken::CheckValid() const
{
    return ((magicBegin_ == MAGIC_BEGIN) && (magicEnd_ == MAGIC_END));
}

TimeStamp SQLiteSingleVerContinueToken::GetQueryBeginTime() const
{
    return GetBeginTimeStamp(timeRanges_);
}

TimeStamp SQLiteSingleVerContinueToken::GetQueryEndTime() const
{
    return GetEndTimeStamp(timeRanges_);
}

TimeStamp SQLiteSingleVerContinueToken::GetDeletedBeginTime() const
{
    return GetBeginTimeStamp(deleteTimeRanges_);
}

TimeStamp SQLiteSingleVerContinueToken::GetDeletedEndTime() const
{
    return GetEndTimeStamp(deleteTimeRanges_);
}

void SQLiteSingleVerContinueToken::SetNextBeginTime(const DeviceID &deviceID, TimeStamp nextBeginTime)
{
    RemovePrevDevAndSetBeginTime(deviceID, nextBeginTime, timeRanges_);
}

const MulDevTimeRanges& SQLiteSingleVerContinueToken::GetTimeRanges()
{
    return timeRanges_;
}

void SQLiteSingleVerContinueToken::SetDeletedNextBeginTime(const DeviceID &deviceID, TimeStamp nextBeginTime)
{
    RemovePrevDevAndSetBeginTime(deviceID, nextBeginTime, deleteTimeRanges_);
}

const MulDevTimeRanges& SQLiteSingleVerContinueToken::GetDeletedTimeRanges() const
{
    return deleteTimeRanges_;
}

void SQLiteSingleVerContinueToken::FinishGetQueryData()
{
    timeRanges_.clear();
}

void SQLiteSingleVerContinueToken::FinishGetDeletedData()
{
    deleteTimeRanges_.clear();
}

bool SQLiteSingleVerContinueToken::IsGetQueryDataFinished() const
{
    return timeRanges_.empty();
}

bool SQLiteSingleVerContinueToken::IsGetDeletedDataFinished() const
{
    return deleteTimeRanges_.empty();
}

bool SQLiteSingleVerContinueToken::IsQuerySync() const
{
    return !queryObject_.empty();
}

QueryObject SQLiteSingleVerContinueToken::GetQuery() const
{
    if (!queryObject_.empty()) {
        return queryObject_.begin()->second;
    }
    return QueryObject{};
}

void SQLiteSingleVerContinueToken::RemovePrevDevAndSetBeginTime(const DeviceID &deviceID, TimeStamp nextBeginTime,
    MulDevTimeRanges &timeRanges)
{
    auto iter = timeRanges.find(deviceID);
    if (iter == timeRanges.end()) {
        return;
    }
    iter = timeRanges.erase(timeRanges.begin(), iter);
    iter->second.first = nextBeginTime;
}

TimeStamp SQLiteSingleVerContinueToken::GetBeginTimeStamp(const MulDevTimeRanges &timeRanges) const
{
    if (timeRanges.empty()) {
        return 0;
    }
    return timeRanges.begin()->second.first;
}

TimeStamp SQLiteSingleVerContinueToken::GetEndTimeStamp(const MulDevTimeRanges &timeRanges) const
{
    if (timeRanges.empty()) {
        return static_cast<TimeStamp>(INT64_MAX);
    }
    return timeRanges.begin()->second.second;
}
}  // namespace DistributedDB
