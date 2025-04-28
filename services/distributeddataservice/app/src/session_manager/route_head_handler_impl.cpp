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
#include "access_check/app_access_check_config_manager.h"
#include "app_id_mapping/app_id_mapping_config_manager.h"
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
constexpr const char *DEFAULT_ACCOUNT_A = "default";
constexpr const char *DEFAULT_ACCOUNT_Z = "ohosAnonymousUid";
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
    auto devInfo = DmAdapter::GetInstance().GetDeviceInfo(session_.targetDeviceId);
    if (devInfo.osType != OH_OS_TYPE) {
        ZLOGD("devicdId:%{public}s is not oh type",
            Anonymous::Change(session_.targetDeviceId).c_str());
        if (!IsTrust()) {
            ZLOGW("distrust app, bundleName:%{public}s", metaData.bundleName.c_str());
            return DistributedDB::DB_ERROR;
        }
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
                        + session_.targetUserIds.size() * sizeof(int) + sizeof(SessionAppId) + session_.appId.size()
                        + sizeof(SessionStoreId) + session_.storeId.size();

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

    SessionAppId *appPair = reinterpret_cast<SessionAppId *>(ptr);
    uint32_t appIdSize = session_.appId.size();
    appPair->len = HostToNet(appIdSize);
    ret = strcpy_s(appPair->appId, appIdSize, session_.appId.c_str());
    if (ret != 0) {
        ZLOGE("strcpy for app id failed, error:%{public}d", errno);
        return false;
    }
    ptr += (sizeof(SessionAppId) + appIdSize);

    uint8_t *end = data + totalLen;
    SessionStoreId *storePair = reinterpret_cast<SessionStoreId *>(ptr);
    uint32_t storeIdSize = session_.storeId.size();
    ret = memcpy_s(storePair->storeId, end - ptr, session_.storeId.data(), storeIdSize);
    if (ret != 0) {
        ZLOGE("strcpy for store id failed, error:%{public}d", errno);
        return false;
    }
    storePair->len = HostToNet(storeIdSize);
    return true;
}

bool RouteHeadHandlerImpl::ParseHeadDataLen(const uint8_t *data, uint32_t totalLen, uint32_t &headSize,
    const std::string &device)
{
    if (data == nullptr) {
        ZLOGE("invalid input data, totalLen:%{public}d", totalLen);
        return false;
    }
    if (!DmAdapter::GetInstance().IsOHOSType(device)) {
        return false;
    }
    RouteHead head = { 0 };
    auto ret = UnPackDataHead(data, totalLen, head);
    headSize = ret ? sizeof(RouteHead) + head.dataLen : 0;
    ZLOGI("unpacked data size:%{public}u, ret:%{public}d", headSize, ret);
    return ret;
}

bool RouteHeadHandlerImpl::ParseStoreInfo(const std::string &accountId, const std::string &label,
    StoreMetaData &storeMeta)
{
    std::vector<std::string> accountIds { accountId, DEFAULT_ACCOUNT_Z, DEFAULT_ACCOUNT_A };
    for (auto &id : accountIds) {
        auto convertedIds =
            AppIdMappingConfigManager::GetInstance().Convert(storeMeta.appId, storeMeta.user);
        const std::string tempTripleLabel =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(id, convertedIds.first,
                storeMeta.storeId, false);
        if (tempTripleLabel == label) {
            ZLOGI("find triple identifier,storeId:%{public}s,storeMeta.bundleName:%{public}s",
                Anonymous::Change(storeMeta.storeId).c_str(), Anonymous::Change(storeMeta.bundleName).c_str());
            storeMeta.appId = convertedIds.first;
            storeMeta.bundleName = convertedIds.first;
            return true;
        }
    }
    return false;
}

bool RouteHeadHandlerImpl::IsTrust()
{
    StoreMetaData metaData;
    auto [appId, storeId] = AppIdMappingConfigManager::GetInstance().Convert(appId_, storeId_);
    metaData.bundleName = appId;
    metaData.appId = appId;
    return AppAccessCheckConfigManager::GetInstance().IsTrust({ metaData.bundleName, metaData.appId });
}

bool RouteHeadHandlerImpl::IsTrust(const std::string &label)
{
    std::vector<StoreMetaData> metaDatas;
    auto prefix = StoreMetaData::GetPrefix({ DmAdapter::GetInstance().GetLocalDevice().uuid });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaDatas)) {
        ZLOGE("get meta failed.");
        return false;
    }

    auto accountId = AccountDelegate::GetInstance()->GetUnencryptedAccountId();
    for (auto storeMeta : metaDatas) {
        if (storeMeta.appId == DistributedData::Bootstrap::GetInstance().GetProcessLabel()) {
            continue;
        }
        if (!ParseStoreInfo(accountId, label, storeMeta)) {
            continue;
        }
        return AppAccessCheckConfigManager::GetInstance().IsTrust({ storeMeta.bundleName, storeMeta.appId });
    }
    ZLOGI("not found app msg:%{public}s", label.c_str());
    return false;
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

std::pair<bool, bool> RouteHeadHandlerImpl::IsAppTrusted(const std::string &label, const std::string &device)
{
    if (DmAdapter::GetInstance().IsOHOSType(device)) {
        return std::make_pair(true, true);
    }
    bool isTrust = IsTrust(label);
    return return std::make_pair(isTrust, false);
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
    SessionPoint peer { .deviceId = session_.sourceDeviceId, .userId = session_.sourceUserId, .appId = session_.appId };
    ZLOGD("valid session:appId:%{public}s, srcDevId:%{public}s, srcUser:%{public}u, trgDevId:%{public}s,",
          session_.appId.c_str(), Anonymous::Change(session_.sourceDeviceId).c_str(),
          session_.sourceUserId, Anonymous::Change(session_.targetDeviceId).c_str());
    for (const auto &item : session_.targetUserIds) {
        local.userId = item;
        if (SessionManager::GetInstance().CheckSession(local, peer)) {
            UserInfo userInfo = { .receiveUser = std::to_string(item) };
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
    const uint8_t *ptr = data;
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

    if (leftSize < sizeof(SessionAppId)) {
        ZLOGE("failed to parse app id, leftSize : %{public}d", leftSize);
        return false;
    }
    const SessionAppId *appId = reinterpret_cast<const SessionAppId *>(ptr);
    auto appIdLen = NetToHost(appId->len);
    if (leftSize - sizeof(SessionAppId) < appIdLen) {
        ZLOGE("failed to parse app id, appIdLen:%{public}d, leftSize:%{public}d.", appIdLen, leftSize);
        return false;
    }
    session_.appId.append(appId->appId, appIdLen);
    leftSize -= (sizeof(SessionAppId) + appIdLen);
    if (leftSize > 0) {
        ptr += (sizeof(SessionAppId) + appIdLen);
        return UnPackStoreId(ptr, leftSize);
    }
    return true;
}

bool RouteHeadHandlerImpl::UnPackStoreId(const uint8_t *data, uint32_t leftSize)
{
    if (leftSize < sizeof(SessionStoreId)) {
        ZLOGE("failed to parse store id, leftSize:%{public}d.", leftSize);
        return false;
    }
    const uint8_t *ptr = data;
    const SessionStoreId *storeId = reinterpret_cast<const SessionStoreId *>(ptr);
    auto storeIdLen = NetToHost(storeId->len);
    if (leftSize - sizeof(SessionStoreId) < storeIdLen) {
        ZLOGE("failed to parse store id, storeIdLen:%{public}d, leftSize:%{public}d.", storeIdLen, leftSize);
        return false;
    }
    session_.storeId = std::string(storeId->storeId, storeIdLen);
    return true;
}
} // namespace OHOS::DistributedData