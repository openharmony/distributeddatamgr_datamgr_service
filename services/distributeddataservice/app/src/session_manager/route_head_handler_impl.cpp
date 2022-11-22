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
#include "route_head_handler_impl.h"

#define LOG_TAG "RouteHeadHandler"

#include "auth_delegate.h"
#include "device_manager_adapter.h"
#include "kvstore_meta_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "securec.h"
#include "upgrade_manager.h"
#include "bootstrap.h"
#include "utils/anonymous.h"
#include "utils/endian_converter.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
using DmAdapter = DistributedData::DeviceManagerAdapter;
constexpr const int ALIGN_WIDTH = 8;
std::shared_ptr<RouteHeadHandler> RouteHeadHandlerImpl::Create(const ExtendInfo &info)
{
    auto handler = std::make_shared<RouteHeadHandlerImpl>(info);
    if (handler == nullptr) {
        ZLOGE("new instance failed");
        return nullptr;
    }
    handler->Init();
    return handler;
}

RouteHeadHandlerImpl::RouteHeadHandlerImpl(const ExtendInfo &info)
    : userId_(info.userId), appId_(info.appId), storeId_(info.storeId), deviceId_(info.dstTarget), headSize_(0)
{
    ZLOGI("init route handler, app:%{public}s, user:%{public}s, peer:%{public}s", appId_.c_str(), userId_.c_str(),
        Anonymous::Change(deviceId_).c_str());
}

void RouteHeadHandlerImpl::Init()
{
    ZLOGD("begin");
    if (deviceId_.empty()) {
        return;
    }
    SessionPoint localPoint { DmAdapter::GetInstance().GetLocalDevice().uuid,
        static_cast<uint32_t>(atoi(userId_.c_str())), appId_, storeId_ };
    session_ = SessionManager::GetInstance().GetSession(localPoint, deviceId_);
    ZLOGD("valid session:appId:%{public}s, srcDevId:%{public}s, srcUser:%{public}u, trgDevId:%{public}s,",
          session_.appId.c_str(), Anonymous::Change(session_.sourceDeviceId).c_str(),
          session_.sourceUserId, Anonymous::Change(session_.targetDeviceId).c_str());
}

DistributedDB::DBStatus RouteHeadHandlerImpl::GetHeadDataSize(uint32_t &headSize)
{
    ZLOGD("begin");
    headSize = 0;
    if (appId_ == Bootstrap::GetInstance().GetProcessLabel()) {
        ZLOGI("meta data permitted");
        return DistributedDB::OK;
    }
    bool flag = false;
    auto peerCap = UpgradeManager::GetInstance().GetCapability(session_.targetDeviceId, flag);
    if (!flag) {
        ZLOGI("get peer cap failed");
        return DistributedDB::DB_ERROR;
    }
    if (peerCap.version == CapMetaData::INVALID_VERSION) {
        // older versions ignore pack extend head
        ZLOGI("ignore older version device");
        return DistributedDB::OK;
    }
    if (!session_.IsValid()) {
        ZLOGI("no valid session to peer device");
        return DistributedDB::DB_ERROR;
    }
    size_t expectSize = sizeof(RouteHead) + sizeof(SessionDevicePair) + sizeof(SessionUserPair)
                        + session_.targetUserIds.size() * sizeof(int) + sizeof(SessionAppId) + session_.appId.size();

    // align message uint width
    headSize = GET_ALIGNED_SIZE(expectSize, ALIGN_WIDTH);
    ZLOGI("packed size:%{public}u", headSize);
    headSize_ = headSize;
    return DistributedDB::OK;
}

DistributedDB::DBStatus RouteHeadHandlerImpl::FillHeadData(uint8_t *data, uint32_t headSize, uint32_t totalLen)
{
    ZLOGD("begin");
    if (headSize != headSize_) {
        ZLOGI("size not match");
        return DistributedDB::DB_ERROR;
    }
    if (headSize_ == 0) {
        ZLOGI("ignore older version device");
        return DistributedDB::OK;
    }
    auto packRet = PackData(data, headSize);
    ZLOGD("pack result:%{public}d", packRet);
    return packRet ? DistributedDB::OK : DistributedDB::DB_ERROR;
}

bool RouteHeadHandlerImpl::PackData(uint8_t *data, uint32_t totalLen)
{
    if (headSize_ > totalLen) {
        ZLOGE("the buffer size is not enough");
        return false;
    }

    auto isOk = PackDataHead(data, headSize_);
    if (isOk) {
        return PackDataBody(data + sizeof(RouteHead), headSize_ - sizeof(RouteHead));
    }
    return false;
}

bool RouteHeadHandlerImpl::PackDataHead(uint8_t *data, uint32_t totalLen)
{
    uint8_t *ptr = data;
    if (headSize_ < sizeof(RouteHead)) {
        return false;
    }
    RouteHead *head = reinterpret_cast<RouteHead *>(ptr);
    head->magic = HostToNet(RouteHead::MAGIC_NUMBER);
    head->version = HostToNet(RouteHead::VERSION);
    head->checkSum = HostToNet(uint64_t(0));
    head->dataLen = HostToNet(uint32_t(totalLen - sizeof(RouteHead)));
    return true;
}

bool RouteHeadHandlerImpl::PackDataBody(uint8_t *data, uint32_t totalLen)
{
    uint8_t *ptr = data;
    SessionDevicePair *devicePair = reinterpret_cast<SessionDevicePair *>(ptr);
    auto ret = strcpy_s(devicePair->sourceId, SessionDevicePair::MAX_DEVICE_ID, session_.sourceDeviceId.c_str());
    if (ret != 0) {
        ZLOGE("strcpy for source device id failed");
        return false;
    }
    ret = strcpy_s(devicePair->targetId, SessionDevicePair::MAX_DEVICE_ID, session_.targetDeviceId.c_str());
    if (ret != 0) {
        ZLOGE("strcpy for target device id failed");
        return false;
    }
    ptr += sizeof(SessionDevicePair);

    SessionUserPair *userPair = reinterpret_cast<SessionUserPair *>(ptr);
    userPair->sourceUserId = HostToNet(session_.sourceUserId);
    userPair->targetUserCount = session_.targetUserIds.size();
    for (size_t i = 0; i < session_.targetUserIds.size(); ++i) {
        *(userPair->targetUserIds + i) = HostToNet(session_.targetUserIds[i]);
    }
    ptr += (sizeof(SessionUserPair) + session_.targetUserIds.size() * sizeof(int));

    SessionAppId *appPair = reinterpret_cast<SessionAppId *>(ptr);
    ptr += sizeof(SessionAppId);

    uint8_t *end = data + totalLen;
    uint32_t appIdSize = session_.appId.size();
    ret = memcpy_s(appPair->appId, end - ptr, session_.appId.data(), appIdSize);
    if (ret != 0) {
        ZLOGE("strcpy for app id failed:%{public}d", ret);
        return false;
    }
    appPair->len = HostToNet(appIdSize);
    return true;
}

bool RouteHeadHandlerImpl::ParseHeadData(
    const uint8_t *data, uint32_t len, uint32_t &headSize, std::vector<std::string> &users)
{
    auto ret = UnPackData(data, len, headSize);
    if (!ret) {
        headSize = 0;
        ZLOGE("unpack data head failed");
        return false;
    }
    ZLOGI("unpacked size:%{public}u", headSize);
    // flip the local and peer ends
    SessionPoint local { .deviceId = session_.targetDeviceId, .appId = session_.appId };
    SessionPoint peer { .deviceId = session_.sourceDeviceId, .userId = session_.sourceUserId, .appId = session_.appId };
    ZLOGD("valid session:appId:%{public}s, srcDevId:%{public}s, srcUser:%{public}u, trgDevId:%{public}s,",
          session_.appId.c_str(), Anonymous::Change(session_.sourceDeviceId).c_str(),
          session_.sourceUserId, Anonymous::Change(session_.targetDeviceId).c_str());
    for (const auto &item : session_.targetUserIds) {
        local.userId = item;
        if (SessionManager::GetInstance().CheckSession(local, peer)) {
            users.emplace_back(std::to_string(item));
        }
    }
    return true;
}

bool RouteHeadHandlerImpl::UnPackData(const uint8_t *data, uint32_t totalLen, uint32_t &unpackedSize)
{
    if (data == nullptr || totalLen < sizeof(RouteHead)) {
        ZLOGE("invalid input data");
        return false;
    }
    unpackedSize = 0;
    RouteHead head = { 0 };
    bool result = UnPackDataHead(data, totalLen, head);
    if (result && head.version == RouteHead::VERSION) {
        auto isOk = UnPackDataBody(data + sizeof(RouteHead), totalLen - sizeof(RouteHead));
        if (isOk) {
            unpackedSize = sizeof(RouteHead) + head.dataLen;
        }
        return isOk;
    }
    return false;
}

bool RouteHeadHandlerImpl::UnPackDataHead(const uint8_t *data, uint32_t totalLen, RouteHead &routeHead)
{
    if (totalLen < sizeof(RouteHead)) {
        ZLOGE("invalid route head len");
        return false;
    }
    const RouteHead *head = reinterpret_cast<const RouteHead *>(data);
    routeHead.magic = NetToHost(head->magic);
    routeHead.version = NetToHost(head->version);
    routeHead.checkSum = NetToHost(head->checkSum);
    routeHead.dataLen = NetToHost(head->dataLen);
    if (routeHead.magic != RouteHead::MAGIC_NUMBER) {
        ZLOGW("not route head data");
        return false;
    }
    if (totalLen - sizeof(RouteHead) < routeHead.dataLen) {
        ZLOGE("invalid route data len");
        return false;
    }
    return true;
}

bool RouteHeadHandlerImpl::UnPackDataBody(const uint8_t *data, uint32_t totalLen)
{
    const uint8_t *ptr = data;
    uint32_t leftSize = totalLen;

    if (leftSize < sizeof(SessionDevicePair)) {
        ZLOGE("failed to parse device pair");
        return false;
    }
    const SessionDevicePair *devicePair = reinterpret_cast<const SessionDevicePair *>(ptr);
    session_.sourceDeviceId =
        std::string(devicePair->sourceId, strnlen(devicePair->sourceId, SessionDevicePair::MAX_DEVICE_ID));
    session_.targetDeviceId =
        std::string(devicePair->targetId, strnlen(devicePair->targetId, SessionDevicePair::MAX_DEVICE_ID));
    ptr += sizeof(SessionDevicePair);
    leftSize -= sizeof(SessionDevicePair);

    if (leftSize < sizeof(SessionUserPair)) {
        ZLOGE("failed to parse user pair");
        return false;
    }
    const SessionUserPair *userPair = reinterpret_cast<const SessionUserPair *>(ptr);
    session_.sourceUserId = NetToHost(userPair->sourceUserId);

    auto userPairSize = sizeof(SessionUserPair) + userPair->targetUserCount * sizeof(uint32_t);
    if (leftSize < userPairSize) {
        ZLOGE("failed to parse user pair, target user");
        return false;
    }
    for (int i = 0; i < userPair->targetUserCount; ++i) {
        session_.targetUserIds.push_back(NetToHost(*(userPair->targetUserIds + i)));
    }
    ptr += userPairSize;
    leftSize -= userPairSize;

    if (leftSize < sizeof(SessionAppId)) {
        ZLOGE("failed to parse app id len");
        return false;
    }
    const SessionAppId *appId = reinterpret_cast<const SessionAppId *>(ptr);
    auto appIdLen = NetToHost(appId->len);
    if (leftSize - sizeof(SessionAppId) < appIdLen) {
        ZLOGE("failed to parse app id");
        return false;
    }
    session_.appId.append(appId->appId, appIdLen);
    return true;
}
} // namespace OHOS::DistributedData