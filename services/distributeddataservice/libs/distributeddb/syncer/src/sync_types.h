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

#ifndef SYNC_TYPES_H
#define SYNC_TYPES_H

#include <cstdint>
#include "query_sync_object.h"
#include "sync_config.h"

namespace DistributedDB {
enum MessageId {
    TIME_SYNC_MESSAGE = 1,
    DATA_SYNC_MESSAGE,
    COMMIT_HISTORY_SYNC_MESSAGE,
    MULTI_VER_DATA_SYNC_MESSAGE,
    VALUE_SLICE_SYNC_MESSAGE,
    LOCAL_DATA_CHANGED,
    ABILITY_SYNC_MESSAGE,
    QUERY_SYNC_MESSAGE,
    CONTROL_SYNC_MESSAGE,
    UNKNOW_MESSAGE,
};

enum SyncModeType {
    PUSH,
    PULL,
    PUSH_AND_PULL,
    AUTO_PUSH,
    AUTO_PULL,
    RESPONSE_PULL,
    QUERY_PUSH,
    QUERY_PULL,
    QUERY_PUSH_PULL,
    SUBSCRIBE_QUERY,
    UNSUBSCRIBE_QUERY,
    AUTO_SUBSCRIBE_QUERY,
    INVALID_MODE
};

enum class SyncType {
    MANUAL_FULL_SYNC_TYPE = 1,
    AUTO_SYNC_TYPE,
    QUERY_SYNC_TYPE,
    INVALID_SYNC_TYPE,
};

enum ControlCmdType {
    SUBSCRIBE_QUERY_CMD,
    UNSUBSCRIBE_QUERY_CMD,
    INVALID_CONTROL_CMD,
};

struct UpdateWaterMark {
    bool normalUpdateMark = false;
    bool deleteUpdateMark = false;
};

struct InternalSyncParma {
    std::vector<std::string> devices;
    int mode = 0;
    bool isQuerySync = false;
    QuerySyncObject syncQuery;
};

constexpr uint32_t SEND_TIME_OUT = 3000; // 3s
constexpr int NOT_SURPPORT_SEC_CLASSIFICATION = 0xff;
constexpr uint8_t QUERY_SYNC_MODE_BASE = SyncModeType::QUERY_PUSH;
constexpr int AUTO_RETRY_TIMES = 3;
constexpr int MANUAL_RETRY_TIMES = 1;
constexpr int TIME_SYNC_WAIT_TIME = 5000; // 5s
constexpr uint64_t MAX_PACKETID = 10000000000; // max packetId
constexpr int NOTIFY_MIN_MTU_SIZE = 30 * 1024; // 30k

constexpr int MAX_SUBSCRIBE_NUM_PER_DEV = 4;
constexpr int MAX_SUBSCRIBE_NUM_PER_DB = 8;
constexpr size_t MAX_DEVICES_NUM = 32;

// index 0 for packetId in data request
// if ack reserve size is 1, reserve is {localWaterMark}
// if ack reserve size is above 2, reserve is {localWaterMark, packetId, deletedWaterMark...}
constexpr uint32_t REQUEST_PACKET_RESERVED_INDEX_PACKETID = 0;
constexpr uint32_t ACK_PACKET_RESERVED_INDEX_PACKETID = 1; // index 1 for packetId
constexpr uint32_t ACK_PACKET_RESERVED_INDEX_LOCAL_WATER_MARK = 0; // index 0 for localWaterMark
constexpr uint32_t ACK_PACKET_RESERVED_INDEX_DELETE_WATER_MARK = 2; // index 2 for deleteDataWaterMark
constexpr uint64_t MAX_TIMESTAMP = INT64_MAX;
constexpr uint8_t REMOVE_DEVICE_DATA_MARK = 1;
constexpr uint8_t SUPPORT_MARK = 1; // used for set is support one ability
}
#endif