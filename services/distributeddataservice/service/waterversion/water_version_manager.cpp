/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "WaterVersionManager"
#include "water_version_manager.h"

#include "checker/checker_manager.h"
#include "cloud/cloud_sync_finished_event.h"
#include "device_matrix.h"
#include "store/auto_cache.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/matrix_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "utils/anonymous.h"
#include "utils/constant.h"

namespace OHOS::DistributedData {
using DMAdapter = DeviceManagerAdapter;
using WaterVersionMetaData = WaterVersionManager::WaterVersionMetaData;

WaterVersionManager &WaterVersionManager::GetInstance()
{
    static WaterVersionManager waterVersionManager;
    return waterVersionManager;
}
WaterVersionManager::WaterVersionManager() : waterVersions_(BUTT)
{
    for (int i = 0; i < BUTT; ++i) {
        waterVersions_[i].SetType(static_cast<Type>(i));
    }
}
WaterVersionManager::~WaterVersionManager() {}

void WaterVersionManager::Init()
{
    std::vector<WaterVersionMetaData> metas;
    auto stores = CheckerManager::GetInstance().GetDynamicStores();
    for (const auto &store : stores) {
        waterVersions_[DYNAMIC].AddKey(Merge(store.bundleName, store.storeId));
    }
    stores = CheckerManager::GetInstance().GetStaticStores();
    for (const auto &store : stores) {
        waterVersions_[STATIC].AddKey(Merge(store.bundleName, store.storeId));
    }
    MetaDataManager::GetInstance().LoadMeta(WaterVersionMetaData::GetPrefix(), metas, true);
    for (auto &meta : metas) {
        if (meta.type < 0 || meta.type > BUTT) {
            ZLOGW("error meta:%{public}s", meta.ToAnonymousString().c_str());
            continue;
        }
        waterVersions_[meta.type].InitWaterVersion(meta);
    }
    EventCenter::GetInstance().Subscribe(CloudEvent::CLOUD_SYNC_FINISHED, [this](const Event &event) {
        auto &cloudSyncFinishedEvent = static_cast<const CloudSyncFinishedEvent &>(event);
        auto storeMeta = cloudSyncFinishedEvent.GetStoreMeta();
        auto store = AutoCache::GetInstance().GetStore(StoreMetaData(storeMeta), {});
        if (store == nullptr) {
            return;
        }
        auto versions = store->GetWaterVersion("");
        for (auto &version : versions) {
            SetWaterVersion(storeMeta.bundleName, storeMeta.storeId, version);
        }
    });
}

std::string WaterVersionManager::GenerateWaterVersion(const std::string &bundleName, const std::string &storeName)
{
    auto type = BUTT;
    CheckerManager::StoreInfo info;
    info.bundleName = bundleName;
    info.storeId = storeName;
    if (CheckerManager::GetInstance().IsDynamic(info)) {
        type = DYNAMIC;
    }
    if (CheckerManager::GetInstance().IsStatic(info)) {
        type = STATIC;
    }
    if (type < 0 || type >= BUTT || bundleName.empty() || storeName.empty()) {
        ZLOGE("invalid args. bundleName:%{public}s, storeName:%{public}s, type:%{public}d", bundleName.c_str(),
            Anonymous::Change(storeName).c_str(), type);
        return "";
    }
    auto [success, meta] = waterVersions_[type].GenerateWaterVersion(Merge(bundleName, storeName));
    return success ? Serializable::Marshall(meta) : "";
}

std::string WaterVersionManager::GetWaterVersion(const std::string &bundleName, const std::string &storeName)
{
    auto type = BUTT;
    CheckerManager::StoreInfo info;
    info.bundleName = bundleName;
    info.storeId = storeName;
    if (CheckerManager::GetInstance().IsDynamic(info)) {
        type = DYNAMIC;
    }
    if (CheckerManager::GetInstance().IsStatic(info)) {
        type = STATIC;
    }
    if (type < 0 || type >= BUTT || bundleName.empty() || storeName.empty()) {
        ZLOGE("invalid args. bundleName:%{public}s, storeName:%{public}s, type:%{public}d", bundleName.c_str(),
            Anonymous::Change(storeName).c_str(), type);
        return "";
    }
    auto [success, meta] = waterVersions_[type].GetWaterVersion(DMAdapter::GetInstance().GetLocalDevice().uuid);
    if (!success) {
        return "";
    }
    auto target = Merge(bundleName, storeName);
    int i = 0;
    for (auto &key : meta.keys) {
        if (key == target) {
            meta.waterVersion = meta.infos[i][i];
            return Serializable::Marshall(meta);
        }
        i++;
    }
    ZLOGE("invalid args. bundleName:%{public}s, storeName:%{public}s, meta:%{public}d", bundleName.c_str(),
        Anonymous::Change(storeName).c_str(), type);
    return "";
}

std::pair<bool, uint64_t> WaterVersionManager::GetVersion(const std::string &deviceId,
    WaterVersionManager::Type type)
{
    if (type < 0 || type >= BUTT || deviceId.empty()) {
        ZLOGE("invalid args, type:%{public}d", type);
        return { false, 0 };
    }
    auto [success, waterVersion] = waterVersions_[type].GetWaterVersion(deviceId);
    return { success, success ? waterVersion.waterVersion : 0 };
}

std::string WaterVersionManager::GetWaterVersion(const std::string &deviceId, WaterVersionManager::Type type)
{
    if (type < 0 || type >= BUTT || deviceId.empty()) {
        ZLOGE("invalid args, type:%{public}d", type);
        return { false, 0 };
    }
    auto [success, waterVersion] = waterVersions_[type].GetWaterVersion(deviceId);
    return success ? Serializable::Marshall(waterVersion) : "";
}

bool WaterVersionManager::SetWaterVersion(const std::string &bundleName, const std::string &storeName,
    const std::string &waterVersion)
{
    WaterVersionMetaData metaData;
    if (!Serializable::Unmarshall(waterVersion, metaData) || metaData.deviceId.empty() || !metaData.IsValid()) {
        ZLOGE("invalid args. meta:%{public}s, bundleName:%{public}s, storeName:%{public}s",
            metaData.ToAnonymousString().c_str(), bundleName.c_str(), Anonymous::Change(storeName).c_str());
        return false;
    }
    if (metaData.deviceId == DMAdapter::GetInstance().GetLocalDevice().uuid) {
        return false;
    }
    return waterVersions_[metaData.type].SetWaterVersion(Merge(bundleName, storeName), metaData);
}

bool WaterVersionManager::DelWaterVersion(const std::string &deviceId)
{
    bool res = true;
    for (auto &waterVersion : waterVersions_) {
        res = res && waterVersion.DelWaterVersion(deviceId);
    }
    return res;
}

bool WaterVersionManager::InitMeta(WaterVersionMetaData &metaData)
{
    metaData.waterVersion = 0;
    std::string uuid = DMAdapter::GetInstance().GetLocalDevice().uuid;
    for (size_t i = 0; i < metaData.keys.size(); ++i) {
        auto key = metaData.keys[i];
        StoreMetaData storeMetaData;
        storeMetaData.user = std::to_string(0);
        std::tie(storeMetaData.bundleName, storeMetaData.storeId) = Split(key);
        storeMetaData.instanceId = 0;
        if (!MetaDataManager::GetInstance().LoadMeta(storeMetaData.GetKey(), storeMetaData, true)) {
            ZLOGE("no store! bundleName:%{public}s, storeName:%{public}s", storeMetaData.bundleName.c_str(),
                storeMetaData.GetStoreAlias().c_str());
            continue;
        }
        auto store = AutoCache::GetInstance().GetStore(storeMetaData, {});
        if (store == nullptr) {
            ZLOGE("store is null! bundleName:%{public}s, storeName:%{public}s", storeMetaData.bundleName.c_str(),
                storeMetaData.GetStoreAlias().c_str());
            continue;
        }
        auto waterVersion = store->GetWaterVersion(uuid);
        WaterVersionMetaData meta;
        if (waterVersion.empty() || !Serializable::Unmarshall(*waterVersion.begin(), meta)) {
            ZLOGE("GetWaterVersion failed! bundleName:%{public}s, storeName:%{public}s, meta:%{public}s",
                storeMetaData.bundleName.c_str(), storeMetaData.GetStoreAlias().c_str(),
                meta.ToAnonymousString().c_str());
            continue;
        }
        if (meta.version != metaData.version || meta.keys != metaData.keys) {
            meta = Upgrade(metaData.keys, meta);
        }
        metaData.infos[i] = meta.infos[i];
    }
    UpdateWaterVersion(metaData);
    return true;
}

void WaterVersionManager::UpdateWaterVersion(WaterVersionMetaData &metaData)
{
    ZLOGI("before update meta:%{public}s", metaData.ToAnonymousString().c_str());
    uint64_t maxVersion = 0;
    uint64_t consistentVersion = 0;
    for (size_t i = 0; i < metaData.keys.size(); ++i) {
        for (size_t j = 0; j < metaData.keys.size(); ++j) {
            maxVersion = std::max(maxVersion, metaData.infos[j][j]);
            if (metaData.infos[i][j] > metaData.infos[j][j]) {
                break;
            }
            if (j == metaData.keys.size() - 1 && metaData.infos[i][i] > consistentVersion) {
                consistentVersion = metaData.infos[i][i];
            }
        }
    }
    if (consistentVersion > metaData.waterVersion || maxVersion < metaData.waterVersion) {
        metaData.waterVersion = consistentVersion;
        SaveMatrix(metaData);
        ZLOGI("after update meta:%{public}s", metaData.ToAnonymousString().c_str());
    } else {
        ZLOGD("after update meta:%{public}s", metaData.ToAnonymousString().c_str());
    }
}

std::string WaterVersionManager::Merge(const std::string &bundleName, const std::string &storeName)
{
    return Constant::Join(bundleName, Constant::KEY_SEPARATOR, { storeName });
}

std::pair<std::string, std::string> WaterVersionManager::Split(const std::string &key)
{
    std::vector<std::string> res;
    size_t pos = 0;
    std::string delim(Constant::KEY_SEPARATOR);
    while (pos < key.size()) {
        size_t found = key.find(delim, pos);
        if (found == std::string::npos) {
            res.push_back(key.substr(pos));
            break;
        }
        res.push_back(key.substr(pos, found - pos));
        pos = found + delim.size();
    }
    // The number of res should more than 1
    if (res.size() <= 1) {
        return { "", "" };
    }
    return { res[0], res[1] };
}

WaterVersionMetaData WaterVersionManager::Upgrade(const std::vector<std::string> &keys,
    const WaterVersionMetaData &meta)
{
    WaterVersionMetaData newMeta;
    newMeta.keys = keys;
    newMeta.version = WaterVersionMetaData::DEFAULT_VERSION;
    newMeta.deviceId = meta.deviceId;
    newMeta.waterVersion = meta.waterVersion;
    newMeta.type = meta.type;
    newMeta.infos = { keys.size(), std::vector<uint64_t>(keys.size(), 0) };
    std::map<size_t, size_t> mp;
    for (size_t i = 0; i < keys.size(); ++i) {
        for (size_t j = 0; j < meta.keys.size(); ++j) {
            if (meta.keys[j] == keys[i]) {
                mp[j] = i;
                continue;
            }
        }
    }
    for (size_t i = 0; i < meta.keys.size(); ++i) {
        if (!mp.count(i)) {
            continue;
        }
        for (size_t j = 0; j < meta.keys.size(); ++j) {
            if (!mp.count(j)) {
                continue;
            }
            newMeta.infos[mp[i]][mp[j]] = meta.infos[i][j];
        }
    }
    return newMeta;
}

void WaterVersionManager::SaveMatrix(const WaterVersionMetaData &metaData)
{
    MatrixMetaData matrixMetaData;
    matrixMetaData.deviceId = metaData.deviceId;
    auto localUuid = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto key = matrixMetaData.deviceId == localUuid ? matrixMetaData.GetKey()
                                                    : matrixMetaData.GetConsistentKey();
    MetaDataManager::GetInstance().LoadMeta(key, matrixMetaData);
    uint16_t version = (metaData.waterVersion & 0xFFF) << 4;
    if (metaData.type == STATIC && version > (matrixMetaData.statics & 0xFFF0)) {
        matrixMetaData.statics = version | (matrixMetaData.statics & 0xF);
    } else if (metaData.type == DYNAMIC && version > (matrixMetaData.dynamic & 0xFFF0)) {
        matrixMetaData.dynamic = version | (matrixMetaData.dynamic & 0xF);
    } else {
        return;
    }
    DeviceMatrix::GetInstance().SetMatrixMeta(matrixMetaData, matrixMetaData.deviceId != localUuid);
}

bool WaterVersionMetaData::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(deviceId)], deviceId);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(waterVersion)], waterVersion);
    SetValue(node[GET_NAME(type)], type);
    SetValue(node[GET_NAME(keys)], keys);
    SetValue(node[GET_NAME(infos)], infos);
    return true;
}

bool WaterVersionMetaData::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(deviceId), deviceId);
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(waterVersion), waterVersion);
    int32_t tmp = -1;
    GetValue(node, GET_NAME(type), tmp);
    if (tmp < 0 || tmp >= BUTT) {
        return false;
    }
    type = static_cast<Type>(tmp);
    GetValue(node, GET_NAME(keys), keys);
    GetValue(node, GET_NAME(infos), infos);
    return true;
}

std::string WaterVersionMetaData::ToAnonymousString() const
{
    json root;
    Marshal(root);
    SetValue(root[GET_NAME(deviceId)], Anonymous::Change(deviceId));
    return root.dump(-1, ' ', false, error_handler_t::replace);
}

std::string WaterVersionMetaData::GetKey() const
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, { deviceId, std::to_string(type) });
}

uint64_t WaterVersionMetaData::GetVersion()
{
    return waterVersion;
}

std::string WaterVersionMetaData::GetPrefix()
{
    return KEY_PREFIX;
}

bool WaterVersionMetaData::IsValid()
{
    if (type < 0 || type >= BUTT) {
        return false;
    }
    if (keys.size() != infos.size()) {
        return false;
    }
    for (const auto &info : infos) {
        if (info.size() != keys.size()) {
            return false;
        }
    }
    return true;
}

void WaterVersionManager::WaterVersion::SetType(WaterVersionManager::Type type)
{
    type_ = type;
}

void WaterVersionManager::WaterVersion::AddKey(const std::string &key)
{
    for (auto &tmp : keys_) {
        if (tmp == key) {
            return;
        }
    }
    keys_.push_back(key);
}

std::pair<bool, WaterVersionMetaData> WaterVersionManager::WaterVersion::GenerateWaterVersion(const std::string &key)
{
    WaterVersionMetaData metaData;
    for (size_t i = 0; i < keys_.size(); ++i) {
        if (keys_[i] != key) {
            continue;
        }
        metaData.type = type_;
        metaData.keys = keys_;
        metaData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
        if (metaData.deviceId.empty()) {
            ZLOGE("GetLocalDevice failed!");
            break;
        }
        metaData.infos = { metaData.keys.size(), std::vector<uint64_t>(metaData.keys.size(), 0) };
        // ensure serial execution
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto status = versions_.Get(metaData.deviceId, metaData);
        if (!status && !MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true)) {
            ZLOGW("no WaterVersion, start InitMeta");
            InitMeta(metaData);
        }
        metaData.waterVersion++;
        if ((metaData.waterVersion & INVALID_VERSION) == INVALID_VERSION) {
            metaData.waterVersion++;
        }
        // It has been initialized above, so there is no need to check for out of bounds
        metaData.infos[i][i] = metaData.waterVersion;
        for (size_t j = 0; j < metaData.keys.size(); ++j) {
            metaData.infos[i][j] = metaData.infos[j][j];
        }
        ZLOGI("generate meta:%{public}s", metaData.ToAnonymousString().c_str());
        if (MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true) &&
            versions_.Set(DMAdapter::GetInstance().GetLocalDevice().uuid, metaData)) {
            SaveMatrix(metaData);
            return { true, metaData };
        }
        return { false, metaData };
    }
    return { false, metaData };
}

std::pair<bool, WaterVersionMetaData> WaterVersionManager::WaterVersion::GetWaterVersion(const std::string &deviceId)
{
    WaterVersionMetaData meta;
    if (versions_.Get(deviceId, meta)) {
        return { true, meta };
    }
    meta.deviceId = deviceId;
    meta.type = type_;
    if (MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        versions_.Set(deviceId, meta);
        return { true, meta };
    }
    ZLOGW("no meta. deviceId:%{public}s", Anonymous::Change(deviceId).c_str());
    return { false, meta };
}

bool WaterVersionManager::WaterVersion::SetWaterVersion(const std::string &key,
    const WaterVersionMetaData &metaData)
{
    if (std::find(keys_.begin(), keys_.end(), key) == keys_.end()) {
        ZLOGE("invalid key. meta:%{public}s", metaData.ToAnonymousString().c_str());
        return false;
    }
    WaterVersionMetaData oldMeta;
    // ensure serial execution
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!versions_.Get(metaData.deviceId, oldMeta) &&
        !MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), oldMeta, true)) {
        ZLOGI("save meta:%{public}s", metaData.ToAnonymousString().c_str());
        oldMeta = metaData;
        UpdateWaterVersion(oldMeta);
        return MetaDataManager::GetInstance().SaveMeta(oldMeta.GetKey(), oldMeta, true) &&
               versions_.Set(metaData.deviceId, oldMeta);
    }
    if (oldMeta.keys.size() != metaData.keys.size() || oldMeta.version != metaData.version) {
        ZLOGI("upgrade meta. old:%{public}s, new:%{public}s", oldMeta.ToAnonymousString().c_str(),
            metaData.ToAnonymousString().c_str());
        oldMeta = metaData;
        for (size_t i = 0; i < metaData.keys.size(); ++i) {
            // Set all unSync items to 0
            if (metaData.keys[i] != key) {
                oldMeta.infos[i] = std::vector<uint64_t>(metaData.keys.size(), 0);
            }
        }
    } else {
        for (size_t i = 0; i < oldMeta.keys.size(); ++i) {
            if (oldMeta.keys[i] == key) {
                oldMeta.infos[i] = metaData.infos[i];
                break;
            }
        }
    }
    UpdateWaterVersion(oldMeta);
    return MetaDataManager::GetInstance().SaveMeta(oldMeta.GetKey(), oldMeta, true) &&
           versions_.Set(metaData.deviceId, oldMeta);
}

bool WaterVersionManager::WaterVersion::DelWaterVersion(const std::string &deviceId)
{
    WaterVersionMetaData meta;
    if (versions_.Get(deviceId, meta)) {
        return versions_.Delete(deviceId) && MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    }
    return true;
}

bool WaterVersionManager::WaterVersion::InitWaterVersion(const WaterVersionMetaData &metaData)
{
    if (keys_ != metaData.keys || metaData.version != WaterVersionMetaData::DEFAULT_VERSION) {
        auto meta = Upgrade(keys_, metaData);
        return MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true) &&
               versions_.Set(meta.deviceId, meta);
    }
    return versions_.Set(metaData.deviceId, metaData);
}
} // namespace OHOS::DistributedData
