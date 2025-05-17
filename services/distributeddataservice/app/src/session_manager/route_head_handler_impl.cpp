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
#include <chrono>
#include <cinttypes>
#include "account/account_delegate.h"
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
using namespace std::chrono;
using DmAdapter = DistributedData::DeviceManagerAdapter;
using DBManager = DistributedDB::KvStoreDelegateManager;
constexpr const int ALIGN_WIDTH = 8;
constexpr const char *DEFAULT_USERID = "0";
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
    ZLOGD("init route handler, app:%{public}s, user:%{public}s, peer:%{public}s", appId_.c_str(), userId_.c_str(),
        Anonymous::Change(deviceId_).c_str());
}

void RouteHeadHandlerImpl::Init()
{
    ZLOGD("begin");
    if (deviceId_.empty()) {
        return;
    }
    if (userId_ != DEFAULT_USERID) {
        if (!DmAdapter::GetInstance().IsOHOSType(deviceId_)) {
            userId_ = DEFAULT_USERID;
        } else {
            StoreMetaData metaData;
            metaData.deviceId = deviceId_;
            metaData.user = DEFAULT_USERID;
            metaData.bundleName = appId_;
            metaData.storeId = storeId_;
            if (MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData)) {
                userId_ = DEFAULT_USERID;
            }
        }
    }
    SessionPoint localPoint { DmAdapter::GetInstance().GetLocalDevice().uuid,
        static_cast<uint32_t>(atoi(userId_.c_str())), appId_, storeId_,
        AccountDelegate::GetInstance()->GetCurrentAccountId() };
    session_ = SessionManager::GetInstance().GetSession(localPoint, deviceId_);
    ZLOGD("valid session:appId:%{public}s, srcDevId:%{public}s, srcUser:%{public}u, trgDevId:%{public}s,",
          session_.appId.c_str(), Anonymous::Change(session_.sourceDeviceId).c_str(),
          session_.sourceUserId, Anonymous::Change(session_.targetDeviceId).c_str());
}

std::string RouteHeadHandlerImpl::GetTargetUserId()
{
    if (!session_.IsValid()) {
        ZLOGE("session has no valid user to peer device, targetDeviceId:%{public}s",
            Anonymous::Change(session_.targetDeviceId).c_str());
        return "";
    }
    return std::to_string(session_.targetUserIds[0]);
}

DistributedDB::DBStatus RouteHeadHandlerImpl::GetHeadDataSize(uint32_t &headSize)
{
    ZLOGD("begin");
    headSize = 0;
    if (appId_ == Bootstrap::GetInstance().GetProcessLabel()) {
        ZLOGI("meta data permitted");
        return DistributedDB::OK;
    }
    auto devInfo = DmAdapter::GetInstance().GetDeviceInfo(session_.targetDeviceId);
    if (devInfo.osType != OH_OS_TYPE) {
        ZLOGD("devicdId:%{public}s is not oh type",
            Anonymous::Change(session_.targetDeviceId).c_str());
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
    size_t expectSize = sizeof(RouteHead) + sizeof(SessionDevicePair) + sizeof(SessionUserPair) +
        session_.targetUserIds.size() * sizeof(int) + sizeof(SessionAppId) + session_.appId.size() +
        sizeof(SessionStoreId) + session_.storeId.size() + sizeof(SessionAccountId) + session_.accountId.size();

    // align message uint width
    headSize = GET_ALIGNED_SIZE(expectSize, ALIGN_WIDTH);
    auto time =
        static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    ZLOGI("packed size:%{public}u times %{public}" PRIu64 ".", headSize, time);
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
        ZLOGE("the buffer size is not enough, headSize:%{public}d, tatalLen:%{public}d",
            headSize_, totalLen);
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
        ZLOGE("strcpy for source device id failed, ret is %{public}d", ret);
        return false;
    }
    ret = strcpy_s(devicePair->targetId, SessionDevicePair::MAX_DEVICE_ID, session_.targetDeviceId.c_str());
    if (ret != 0) {
        ZLOGE("strcpy for target device id failed, ret is %{public}d", ret);
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

    const uint8_t *end = data + totalLen;
    if (!PackAppId(&ptr, end) || !PackStoreId(&ptr, end) || !PackAccountId(&ptr, end)) {
        return false;
    }
    return true;
}

bool RouteHeadHandlerImpl::PackAppId(uint8_t **data, const uint8_t *end)
{
    SessionAppId *appPair = reinterpret_cast<SessionAppId *>(*data);
    uint32_t appIdSize = session_.appId.size();
    appPair->len = HostToNet(appIdSize);
    *data += sizeof(SessionAppId);
    auto ret = memcpy_s(appPair->appId, end - *data, session_.appId.c_str(), appIdSize);
    if (ret != EOK) {
        ZLOGE("memcpy for app id failed, ret is %{public}d, leftSize is %{public}u, appIdSize is %{public}u",
            ret, static_cast<uint32_t>(end - *data), appIdSize);
        return false;
    }
    *data += appIdSize;
    return true;
}

bool RouteHeadHandlerImpl::PackStoreId(uint8_t **data, const uint8_t *end)
{
    SessionStoreId *storePair = reinterpret_cast<SessionStoreId *>(*data);
    uint32_t storeIdSize = session_.storeId.size();
    storePair->len = HostToNet(storeIdSize);
    *data += sizeof(SessionStoreId);
    auto ret = memcpy_s(storePair->storeId, end - *data, session_.storeId.data(), storeIdSize);
    if (ret != EOK) {
        ZLOGE("memcpy for store id failed, ret is %{public}d, leftSize is %{public}u, storeIdSize is %{public}u",
            ret, static_cast<uint32_t>(end - *data), storeIdSize);
        return false;
    }
    *data += storeIdSize;
    return true;
}

bool RouteHeadHandlerImpl::PackAccountId(uint8_t **data, const uint8_t *end)
{
    SessionAccountId *accountPair = reinterpret_cast<SessionAccountId *>(*data);
    uint32_t accountIdSize = session_.accountId.size();
    accountPair->len = HostToNet(accountIdSize);
    *data += sizeof(SessionAccountId);
    auto ret = memcpy_s(accountPair->accountId, end - *data, session_.accountId.data(), accountIdSize);
    if (ret != EOK) {
        ZLOGE("memcpy for account id failed, ret is %{public}d, leftSize is %{public}u, storeIdSize is %{public}u",
            ret, static_cast<uint32_t>(end - *data), accountIdSize);
        return false;
    }
    return true;
}

bool RouteHeadHandlerImpl::ParseHeadDataLen(const uint8_t *data, uint32_t totalLen, uint32_t &headSize)
{
    if (data == nullptr) {
        ZLOGE("invalid input data, totalLen:%{public}d", totalLen);
        return false;
    }
    RouteHead head = { 0 };
    auto ret = UnPackDataHead(data, totalLen, head);
    headSize = ret ? sizeof(RouteHead) + head.dataLen : 0;
    ZLOGI("unpacked data size:%{public}u, ret:%{public}d", headSize, ret);
    return ret;
}

std::string RouteHeadHandlerImpl::ParseStoreId(const std::string &deviceId, const std::string &label)
{
    if (!session_.storeId.empty()) {
        return session_.storeId;
    }
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ deviceId });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        return "";
    }
    for (const auto &storeMeta : metaData) {
        auto labelTag = DBManager::GetKvStoreIdentifier("", storeMeta.appId, storeMeta.storeId, true);
        if (labelTag != label) {
            continue;
        }
        return storeMeta.storeId;
    }
    return "";
}

bool RouteHeadHandlerImpl::ParseHeadDataUser(const uint8_t *data, uint32_t totalLen, const std::string &label,
    std::vector<UserInfo> &userInfos)
{
    uint32_t headSize = 0;
    auto ret = UnPackData(data, totalLen, headSize);
    if (!ret) {
        return false;
    }
    auto time = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    ZLOGI("unpacked size:%{public}u times %{public}" PRIu64 ".", headSize, time);

    if (DmAdapter::GetInstance().IsOHOSType(session_.sourceDeviceId)) {
        auto storeId = ParseStoreId(session_.targetDeviceId, label);
        if (!storeId.empty() && std::to_string(session_.sourceUserId) == DEFAULT_USERID) {
            StoreMetaData metaData;
            metaData.deviceId = session_.targetDeviceId;
            metaData.user = DEFAULT_USERID;
            metaData.bundleName = session_.appId;
            metaData.storeId = std::move(storeId);
            if (!MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData)) {
                int foregroundUserId = 0;
                AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
                UserInfo userInfo = { .receiveUser = std::to_string(foregroundUserId) };
                userInfos.emplace_back(userInfo);
                return true;
            }
        }
    }

    // flip the local and peer ends
    SessionPoint local { .deviceId = session_.targetDeviceId, .appId = session_.appId };
    SessionPoint peer { .deviceId = session_.sourceDeviceId, .userId = session_.sourceUserId, .appId = session_.appId,
        .accountId = session_.accountId };
    ZLOGD("valid session:appId:%{public}s, srcDevId:%{public}s, srcUser:%{public}u, trgDevId:%{public}s,",
          session_.appId.c_str(), Anonymous::Change(session_.sourceDeviceId).c_str(),
          session_.sourceUserId, Anonymous::Change(session_.targetDeviceId).c_str());
    bool flag = false;
    auto peerCap = UpgradeManager::GetInstance().GetCapability(session_.sourceDeviceId, flag);
    if (!flag) {
        ZLOGI("get peer cap failed, peer deviceId:%{public}s", Anonymous::Change(session_.sourceDeviceId).c_str());
        return false;
    }
    bool accountFlag = peerCap.version >= CapMetaData::ACCOUNT_VERSION;
    for (const auto &item : session_.targetUserIds) {
        local.userId = item;
        if (SessionManager::GetInstance().CheckSession(local, peer, accountFlag)) {
            UserInfo userInfo = { .receiveUser = std::to_string(item), .sendUser = std::to_string(peer.userId) };
            userInfos.emplace_back(userInfo);
        }
    }
    return true;
}

bool RouteHeadHandlerImpl::UnPackData(const uint8_t *data, uint32_t totalLen, uint32_t &unpackedSize)
{
    if (data == nullptr || totalLen < sizeof(RouteHead)) {
        ZLOGE("invalid input data, totalLen:%{public}d", totalLen);
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
        ZLOGE("invalid route head len:%{public}d", totalLen);
        return false;
    }
    const RouteHead *head = reinterpret_cast<const RouteHead *>(data);
    routeHead.magic = NetToHost(head->magic);
    routeHead.version = NetToHost(head->version);
    routeHead.checkSum = NetToHost(head->checkSum);
    routeHead.dataLen = NetToHost(head->dataLen);
    if (routeHead.magic != RouteHead::MAGIC_NUMBER) {
        ZLOGD("not route head data");
        return false;
    }
    if (totalLen - sizeof(RouteHead) < routeHead.dataLen) {
        ZLOGE("invalid route data len:%{public}d, totalLen:%{public}d.", routeHead.dataLen, totalLen);
        return false;
    }
    return true;
}

bool RouteHeadHandlerImpl::UnPackDataBody(const uint8_t *data, uint32_t totalLen)
{
    uint8_t *ptr = const_cast<uint8_t *>(data);
    uint32_t leftSize = totalLen;

    if (leftSize < sizeof(SessionDevicePair)) {
        ZLOGE("failed to parse device pair, leftSize : %{public}d", leftSize);
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
        ZLOGE("failed to parse user pair, leftSize : %{public}d", leftSize);
        return false;
    }
    const SessionUserPair *userPair = reinterpret_cast<const SessionUserPair *>(ptr);
    session_.sourceUserId = NetToHost(userPair->sourceUserId);

    auto userPairSize = sizeof(SessionUserPair) + userPair->targetUserCount * sizeof(uint32_t);
    if (leftSize < userPairSize) {
        ZLOGE("failed to parse user pair, target user, leftSize : %{public}d", leftSize);
        return false;
    }
    for (int i = 0; i < userPair->targetUserCount; ++i) {
        session_.targetUserIds.push_back(NetToHost(*(userPair->targetUserIds + i)));
    }
    ptr += userPairSize;
    leftSize -= userPairSize;

    if (!UnPackAppId(&ptr, leftSize) || !UnPackStoreId(&ptr, leftSize) || !UnPackAccountId(&ptr, leftSize)) {
        return false;
    }
    return true;
}

bool RouteHeadHandlerImpl::UnPackAppId(uint8_t **data, uint32_t leftSize)
{
    if (leftSize < sizeof(SessionAppId)) {
        ZLOGE("failed to parse app id, leftSize:%{public}d.", leftSize);
        return false;
    }
    const SessionAppId *appId = reinterpret_cast<const SessionAppId *>(*data);
    auto appIdLen = NetToHost(appId->len);
    if (leftSize - sizeof(SessionAppId) < appIdLen) {
        ZLOGE("failed to parse app id, appIdLen:%{public}d, leftSize:%{public}d.", appIdLen, leftSize);
        return false;
    }
    session_.appId = std::string(appId->appId, appIdLen);
    leftSize -= (sizeof(SessionAppId) + appIdLen);
    *data += (sizeof(SessionAppId) + appIdLen);
    return true;
}

bool RouteHeadHandlerImpl::UnPackStoreId(uint8_t **data, uint32_t leftSize)
{
    if (leftSize < sizeof(SessionStoreId)) {
        ZLOGE("failed to parse store id, leftSize:%{public}d.", leftSize);
        return false;
    }
    const SessionStoreId *storeId = reinterpret_cast<const SessionStoreId *>(*data);
    auto storeIdLen = NetToHost(storeId->len);
    if (leftSize - sizeof(SessionStoreId) < storeIdLen) {
        ZLOGE("failed to parse store id, storeIdLen:%{public}d, leftSize:%{public}d.", storeIdLen, leftSize);
        return false;
    }
    session_.storeId = std::string(storeId->storeId, storeIdLen);
    leftSize -= (sizeof(SessionAppId) + storeIdLen);
    *data += (sizeof(SessionAppId) + storeIdLen);
    return true;
}

bool RouteHeadHandlerImpl::UnPackAccountId(uint8_t **data, uint32_t leftSize)
{
    if (leftSize < sizeof(SessionAccountId)) {
        ZLOGE("failed to parse account id, leftSize:%{public}d.", leftSize);
        return false;
    }
    const SessionAccountId *accountId = reinterpret_cast<const SessionAccountId *>(*data);
    auto accountIdLen = NetToHost(accountId->len);
    if (leftSize - sizeof(SessionAccountId) < accountIdLen) {
        ZLOGE("failed to parse account id, accountIdLen:%{public}d, leftSize:%{public}d.", accountIdLen, leftSize);
        return false;
    }
    session_.accountId = std::string(accountId->accountId, accountIdLen);
    return true;
}
} // namespace OHOS::DistributedData