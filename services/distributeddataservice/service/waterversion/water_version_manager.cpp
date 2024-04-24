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
#define LOG_TAG "DeviceMatrix"
#include "water_version_manager.h"

#include "checker/checker_manager.h"
#include "store/auto_cache.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "utils/anonymous.h"
#include "utils/constant.h"

namespace OHOS::DistributedData {
using DMAdapter = DeviceManagerAdapter;

WaterVersionManager &WaterVersionManager::GetInstance()
{
    static WaterVersionManager waterVersionManager;
    return waterVersionManager;
}
WaterVersionManager::WaterVersionManager()
{
    std::vector<WaterVersionMetaData> metas;
    auto stores = CheckerManager::GetInstance().GetDynamicStores();
    for (const auto &store : stores) {
        dynamicKeys_.push_back(Merge(store.bundleName, store.storeId));
    }
    stores = CheckerManager::GetInstance().GetStaticStores();
    for (const auto &store : stores) {
        staticKeys_.push_back(Merge(store.bundleName, store.storeId));
    }
    MetaDataManager::GetInstance().LoadMeta(WaterVersionMetaData::GetPrefix(), metas, true);
    for (auto &meta : metas) {
        if (meta.type == DYNAMIC) {
            if (dynamicKeys_ != meta.keys || meta.version != WaterVersionMetaData::DEFAULT_VERSION) {
                meta = Upgrade(dynamicKeys_, meta);
            }
            dynamicVersions_.Set(meta.deviceId, meta);
            MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
        }
        if (meta.type == STATIC) {
            if (staticKeys_ != meta.keys || meta.version != WaterVersionMetaData::DEFAULT_VERSION) {
                meta = Upgrade(staticKeys_, meta);
            }
            staticVersions_.Set(meta.deviceId, meta);
            MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
        }
    }
}
std::string WaterVersionManager::GenerateWaterVersion(const std::string &bundleName, const std::string &storeName)
{
    WaterVersionMetaData meta;
    std::string key = Merge(bundleName, storeName);
    for (size_t i = 0; i < staticKeys_.size(); ++i) {
        if (staticKeys_[i] == key) {
            meta.type = STATIC;
            meta.keys = staticKeys_;
            return InnerGenerate(i, staticVersions_, meta) ? Serializable::Marshall(meta) : "";
        }
    }
    for (size_t i = 0; i < dynamicKeys_.size(); ++i) {
        if (dynamicKeys_[i] == key) {
            meta.type = DYNAMIC;
            meta.keys = dynamicKeys_;
            return InnerGenerate(i, dynamicVersions_, meta) ? Serializable::Marshall(meta) : "";
        }
    }
    ZLOGE("invalid param. bundleName:%{public}s, storeName:%{public}s", bundleName.c_str(), storeName.c_str());
    return "";
}

bool WaterVersionManager::GetWaterVersion(const std::string &deviceId, WaterVersionManager::Type type,
    std::string &waterVersion)
{
    WaterVersionMetaData meta;
    if (InnerGetWaterVersion(deviceId, type, meta)) {
        waterVersion = Serializable::Marshall(meta);
        return true;
    }
    return false;
}
bool WaterVersionManager::GetWaterVersion(const std::string &deviceId, WaterVersionManager::Type type,
    uint64_t &waterVersion)
{
    WaterVersionMetaData meta;
    if (InnerGetWaterVersion(deviceId, type, meta)) {
        waterVersion = meta.GetVersion();
        return true;
    }
    return false;
}

bool WaterVersionManager::SetWaterVersion(const std::string &bundleName, const std::string &storeName,
    const std::string &waterVersion)
{
    WaterVersionMetaData metaData;
    if (!Serializable::Unmarshall(waterVersion, metaData) || metaData.deviceId.empty() || !metaData.IsValid() ||
        metaData.deviceId == DMAdapter::GetInstance().GetLocalDevice().uuid) {
        ZLOGE("invalid param. meta:%{public}s, bundleName:%{public}s, storeName:%{public}s",
            metaData.ToAnonymousString().c_str(), bundleName.c_str(), Anonymous::Change(storeName).c_str());
        return false;
    }

    std::string key = Merge(bundleName, storeName);
    for (size_t i = 0; i < staticKeys_.size(); ++i) {
        if (metaData.type != STATIC) {
            break;
        }
        if (staticKeys_[i] == key) {
            return InnerSetWaterVersion(key, metaData, staticVersions_);
        }
    }
    for (size_t i = 0; i < dynamicKeys_.size(); ++i) {
        if (metaData.type != DYNAMIC) {
            break;
        }
        if (dynamicKeys_[i] == key) {
            return InnerSetWaterVersion(key, metaData, dynamicVersions_);
        }
    }
    ZLOGE("error key. deviceId:%{public}s, bundleName:%{public}s, storeName:%{public}s",
        Anonymous::Change(metaData.deviceId).c_str(), bundleName.c_str(), Anonymous::Change(storeName).c_str());
    return false;
}

bool WaterVersionManager::DelWaterVersion(const std::string &deviceId)
{
    WaterVersionMetaData meta;
    if (dynamicVersions_.Get(deviceId, meta)) {
        dynamicVersions_.Delete(deviceId);
        MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    }
    if (staticVersions_.Get(deviceId, meta)) {
        staticVersions_.Delete(deviceId);
        MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    }
    return true;
}

bool WaterVersionManager::InnerGenerate(int index, LRUBucket<std::string, WaterVersionMetaData> &bucket,
    WaterVersionMetaData &metaData)
{
    metaData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    if (metaData.deviceId.empty()) {
        ZLOGE("GetLocalDevice failed!");
        return false;
    }
    metaData.infos = { metaData.keys.size(), std::vector<uint64_t>(metaData.keys.size(), 0) };
    // ensure serial execution
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    auto status = bucket.Get(metaData.deviceId, metaData);
    if (!status && !MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true)) {
        ZLOGW("no WaterVersion, start InitMeta");
        InitMeta(metaData);
    }
    metaData.waterVersion++;
    // It has been initialized above, so there is no need to check for out of bounds
    metaData.infos[index][index] = metaData.waterVersion;
    for (size_t i = 0; i < metaData.keys.size(); ++i) {
        metaData.infos[index][i] = metaData.infos[i][i];
    }
    MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true);
    ZLOGI("generate meta:%{public}s", metaData.ToAnonymousString().c_str());
    return bucket.Set(DMAdapter::GetInstance().GetLocalDevice().uuid, metaData);
}

bool WaterVersionManager::InnerGetWaterVersion(const std::string &deviceId, WaterVersionManager::Type type,
    WaterVersionManager::WaterVersionMetaData &meta)
{
    if (deviceId.empty() || type < 0 || type > DYNAMIC) {
        ZLOGE("invalid param. deviceId:%{public}s, type:%{public}d", Anonymous::Change(deviceId).c_str(), type);
        return false;
    }
    meta.type = type;
    meta.deviceId = deviceId;
    bool flag = false;
    switch (type) {
        case DYNAMIC:
            flag = dynamicVersions_.Get(deviceId, meta);
            break;
        case STATIC:
            flag = staticVersions_.Get(deviceId, meta);
            break;
        default:
            ZLOGE("error type:%{public}d. deviceId:%{public}s", type, Anonymous::Change(deviceId).c_str());
            return false;
    }
    if (!flag && MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        type == DYNAMIC ? dynamicVersions_.Set(deviceId, meta) : staticVersions_.Set(deviceId, meta);
    }
    ZLOGI("get meta:%{public}s, flag:%{public}d", meta.ToAnonymousString().c_str(), flag);
    return true;
}

bool WaterVersionManager::InnerSetWaterVersion(const std::string &key,
    const WaterVersionMetaData &metaData, LRUBucket<std::string, WaterVersionMetaData> &bucket)
{
    WaterVersionMetaData oldMeta;
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    bool success = bucket.Get(metaData.deviceId, oldMeta);
    if (!success && !MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), oldMeta, true)) {
        ZLOGI("save meta:%{public}s", metaData.ToAnonymousString().c_str());
        return MetaDataManager::GetInstance().SaveMeta(oldMeta.GetKey(), metaData, true) &&
               bucket.Set(metaData.deviceId, metaData);
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
           bucket.Set(metaData.deviceId, oldMeta);
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
                storeMetaData.storeId.c_str());
            continue;
        }
        auto store = AutoCache::GetInstance().GetStore(storeMetaData, {});
        if (store == nullptr) {
            ZLOGE("store is null! bundleName:%{public}s, storeName:%{public}s", storeMetaData.bundleName.c_str(),
                storeMetaData.storeId.c_str());
            continue;
        }
        auto waterVersion = store->GetWaterVersion(uuid);
        WaterVersionMetaData meta;
        if (waterVersion.empty() || !Serializable::Unmarshall(waterVersion, meta)) {
            ZLOGE("GetWaterVersion failed! bundleName:%{public}s, storeName:%{public}s, meta:%{public}s",
                storeMetaData.bundleName.c_str(), storeMetaData.storeId.c_str(), meta.ToAnonymousString().c_str());
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
void WaterVersionManager::UpdateWaterVersion(WaterVersionManager::WaterVersionMetaData &metaData) const
{
    ZLOGI("before update meta:%{public}s", metaData.ToAnonymousString().c_str());
    for (size_t i = 0; i < metaData.keys.size(); ++i) {
        for (size_t j = 0; j < metaData.keys.size(); ++j) {
            if (metaData.infos[i][j] > metaData.infos[j][j]) {
                break;
            }
            if (j == metaData.keys.size() - 1 && metaData.infos[i][i] > metaData.waterVersion) {
                metaData.waterVersion = metaData.infos[i][i];
            }
        }
    }
    ZLOGI("after update meta:%{public}s", metaData.ToAnonymousString().c_str());
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
    if (res.size() < 2) {
        return { "", "" };
    }
    return { res[0], res[1] };
}

WaterVersionManager::WaterVersionMetaData WaterVersionManager::Upgrade(const std::vector<std::string> &keys,
    const WaterVersionManager::WaterVersionMetaData &meta)
{
    WaterVersionMetaData newMeta;
    newMeta.keys = keys;
    newMeta.version = WaterVersionMetaData::DEFAULT_VERSION;
    newMeta.deviceId = meta.deviceId;
    newMeta.waterVersion = meta.waterVersion;
    newMeta.type = meta.type;
    newMeta.infos = { keys.size(), std::vector<uint64_t>(keys.size(), 0) };
    std::map<int, int> mp;
    for (size_t i = 0; i < keys.size(); ++i) {
        for (size_t j = 0; j < meta.keys.size(); ++j) {
            if (meta.keys[j] == keys[i]) {
                mp[j] = i;
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
bool WaterVersionManager::WaterVersionMetaData::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(deviceId)], deviceId);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(waterVersion)], waterVersion);
    SetValue(node[GET_NAME(type)], type);
    SetValue(node[GET_NAME(keys)], keys);
    SetValue(node[GET_NAME(infos)], infos);
    return true;
}
bool WaterVersionManager::WaterVersionMetaData::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(deviceId), deviceId);
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(waterVersion), waterVersion);
    int32_t tmp = -1;
    GetValue(node, GET_NAME(type), tmp);
    if (tmp < 0 || tmp > DYNAMIC) {
        return false;
    }
    type = static_cast<Type>(tmp);
    GetValue(node, GET_NAME(keys), keys);
    GetValue(node, GET_NAME(infos), infos);
    return true;
}
std::string WaterVersionManager::WaterVersionMetaData::ToAnonymousString() const
{
    json root;
    Marshal(root);
    SetValue(root[GET_NAME(deviceId)], Anonymous::Change(deviceId));
    return root.dump(-1, ' ', false, error_handler_t::replace);
}
std::string WaterVersionManager::WaterVersionMetaData::GetKey() const
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, { deviceId, std::to_string(type) });
}
uint64_t WaterVersionManager::WaterVersionMetaData::GetVersion()
{
    return waterVersion;
}
std::string WaterVersionManager::WaterVersionMetaData::GetPrefix()
{
    return KEY_PREFIX;
}

bool WaterVersionManager::WaterVersionMetaData::IsValid()
{
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
} // namespace OHOS::DistributedData
