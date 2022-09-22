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

#ifndef DISTRIBUTEDDATAMGR_ROUTE_HEAD_HANDLER_H
#define DISTRIBUTEDDATAMGR_ROUTE_HEAD_HANDLER_H
#include "process_communicator_impl.h"
#include "route_head_handler.h"
#include "serializable/serializable.h"
#include "session_manager.h"

namespace OHOS::DistributedData {
template<typename T> constexpr T GET_ALIGNED_SIZE(T x, int alignWidth)
{
    return (x + (alignWidth - 1)) & ~(alignWidth - 1);
}

#pragma pack(1)
// format: head + device pair + user pair + appid
struct RouteHead {
    static constexpr uint16_t MAGIC_NUMBER = 0x8421;
    static constexpr uint16_t VERSION = 0x1;
    uint16_t magic = MAGIC_NUMBER;
    uint16_t version = VERSION;
    uint64_t checkSum;
    uint32_t dataLen;
};

struct SessionDevicePair {
    static constexpr int32_t MAX_DEVICE_ID = 65;
    char sourceId[MAX_DEVICE_ID];
    char targetId[MAX_DEVICE_ID];
};

struct SessionUserPair {
    uint32_t sourceUserId;
    uint8_t targetUserCount;
    uint32_t targetUserIds[0];
};

struct SessionAppId {
    uint32_t len;
    char appId[0];
};
#pragma pack()

class RouteHeadHandlerImpl : public DistributedData::RouteHeadHandler {
public:
    static std::shared_ptr<RouteHeadHandler> Create(const ExtendInfo &info);
    explicit RouteHeadHandlerImpl(const ExtendInfo &info);
    DBStatus GetHeadDataSize(uint32_t &headSize) override;
    DBStatus FillHeadData(uint8_t *data, uint32_t headSize, uint32_t totalLen) override;
    bool ParseHeadData(const uint8_t *data, uint32_t len, uint32_t &headSize, std::vector<std::string> &users) override;

private:
    void Init();
    bool PackData(uint8_t *data, uint32_t totalLen);
    bool PackDataHead(uint8_t *data, uint32_t totalLen);
    bool PackDataBody(uint8_t *data, uint32_t totalLen);
    bool UnPackData(const uint8_t *data, uint32_t totalLen, uint32_t &unpackedSize);
    bool UnPackDataHead(const uint8_t *data, uint32_t totalLen, RouteHead &routeHead);
    bool UnPackDataBody(const uint8_t *data, uint32_t totalLen);

    std::string userId_;
    std::string appId_;
    std::string storeId_;
    std::string deviceId_;
    Session session_;
    uint32_t headSize_;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_ROUTE_HEAD_HANDLER_H
