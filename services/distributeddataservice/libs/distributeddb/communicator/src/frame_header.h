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

#ifndef FRAMEHEADER_H
#define FRAMEHEADER_H

#include <cstdint>
#include "communicator_type_define.h"

namespace DistributedDB {
/*
 * packetType: Bit0:   FragmentFlag: 1: Fragmented 0: Not Fragmented
 *             Bit1~3: Reserved
 *             Bit4~7: FrameType
 */
struct CommPhyHeader {
    uint16_t magic = 0;         // Magic code to discern byte stream
    uint16_t version = 0;       // Version to differentiate fields layout
    uint32_t packetLen = 0;     // Length of total packet, include CommHeader and Padding
    uint64_t checkSum = 0;      // Check sum of data that follows CommPhyHeader
    uint64_t sourceId = 0;      // Indicate where this packet from
    uint32_t frameId = 0;       // FrameId to identify frame
    uint8_t packetType = 0;     // Some bits works individually, the high four bits indicates frameType
    uint8_t paddingLen = 0;     // Unit byte, range from 0 to 7.
    uint16_t dbIntVer = 0;      // Auxiliary info to help recognize db version in the future
};

/*
 * Whether a physical packet contains CommPhyOptHeader depend on FragmentFlag of packetType in CommPhyHeader
 */
struct CommPhyOptHeader {
    uint32_t frameLen = 0;      // Indicate length of frame before fragmentation. Frame include CommHeader no padding
    uint16_t fragCount = 0;     // Indicate how many fragments this frame is divided into
    uint16_t fragNo = 0;        // Indicate which fragment this packet is. start from 0.
};

/*
 * Whether a physical packet contains CommDivergeHeader depend on FrameType of packetType in CommPhyHeader
 */
struct CommDivergeHeader {
    uint16_t version = 0;       // Version to differentiate fields layout
    uint16_t reserved = 0;      // Reserved for future usage
    uint32_t payLoadLen = 0;    // Indicate length of data that follows CommDivergeHeader
    uint8_t commLabel[COMM_LABEL_LENGTH] = {0}; // Indicate which communicator to hand out this frame
};

/*
 * MessageHeader used to describe a message
 */
struct MessageHeader {
    uint16_t version = 0;       // Version to differentiate fields layout
    uint16_t messageType = 0;   // Distinguish request/response/notify
    uint32_t messageId = 0;     // Indicate message command
    uint32_t sessionId = 0;     // For matching request and response
    uint32_t sequenceId = 0;    // Sequence of message
    uint32_t errorNo = 0;       // Indicate no error when zero
    uint32_t dataLen = 0;       // Indicate length of data that follows MessageHeader
};
} // namespace DistributedDB

#endif // FRAMEHEADER_H
