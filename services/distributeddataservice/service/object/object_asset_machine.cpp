/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "ObjectAssetMachine"
#include "object_asset_machine.h"

#include <utility>
#include <utils/anonymous.h>

#include "cloud/change_event.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "object_asset_loader.h"
#include "snapshot/bind_event.h"
#include "store/auto_cache.h"

namespace OHOS {
namespace DistributedObject {
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

constexpr static const char* SQL_AND = " = ? and ";
constexpr static const int32_t AND_SIZE = 5;
static int32_t DoTransfer(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset);

static int32_t ChangeAssetToNormal(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset);

static int32_t CompensateSync(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset);

static int32_t CompensateTransferring(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset);

static int32_t SaveNewAsset(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset);

static int32_t Recover(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset);

static int32_t UpdateStore(ChangedAssetInfo& changedAsset);

static AutoCache::Store GetStore(ChangedAssetInfo& changedAsset);
static VBuckets GetMigratedData(AutoCache::Store& store, AssetBindInfo& assetBindInfo, const Asset& newAsset);
static void MergeAssetData(VBucket& record, const Asset& newAsset, const AssetBindInfo& assetBindInfo);
static void MergeAsset(Asset& oldAsset, const Asset& newAsset);
static std::string BuildSql(const AssetBindInfo& bindInfo, Values& args);
static BindEvent::BindEventInfo MakeBindInfo(ChangedAssetInfo& changedAsset);

static const DFAAction AssetDFA[STATUS_BUTT][EVENT_BUTT] = {
    {
        // STATUS_STABLE
        { STATUS_TRANSFERRING, nullptr, (Action)DoTransfer }, // remote_changed
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },       // transfer_finished
        { STATUS_UPLOADING, nullptr, nullptr },               // upload
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },       // upload_finished
        { STATUS_DOWNLOADING, nullptr, nullptr },             // download
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },       // upload_finished
    },
    {
        // TRANSFERRING
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset },        // remote_changed
        { STATUS_STABLE, nullptr, nullptr },                            // transfer_finished
        { STATUS_WAIT_UPLOAD, nullptr, (Action)ChangeAssetToNormal },   // upload
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },                 // upload_finished
        { STATUS_WAIT_DOWNLOAD, nullptr, (Action)ChangeAssetToNormal }, // download
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },                 // upload_finished
    },
    {
        // DOWNLOADING
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset }, // remote_changed
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // transfer_finished
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // upload
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // upload_finished
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // download
        { STATUS_STABLE, nullptr, nullptr },                     // download_finished
    },
    {
        // STATUS_UPLOADING
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset }, // remote_changed
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // transfer_finished
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // upload
        { STATUS_STABLE, nullptr, nullptr },                     // upload_finished
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // download
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },          // download_finished
    },
    {
        // STATUS_WAIT_TRANSFER
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset },        // remote_changed
        { STATUS_STABLE, nullptr, (Action)CompensateTransferring },     // transfer_finished
        { STATUS_WAIT_UPLOAD, nullptr, (Action)ChangeAssetToNormal },   // upload
        { STATUS_STABLE, nullptr, (Action)CompensateTransferring },     // upload_finished
        { STATUS_WAIT_DOWNLOAD, nullptr, (Action)ChangeAssetToNormal }, // download
        { STATUS_STABLE, nullptr, (Action)CompensateTransferring },     // download_finished
    },
    {
        // STATUS_WAIT_UPLOAD
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset },        // remote_changed
        { STATUS_STABLE, nullptr, (Action)CompensateSync },             // transfer_finished
        { STATUS_WAIT_UPLOAD, nullptr, (Action)ChangeAssetToNormal },   // upload
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },                 // upload_finished
        { STATUS_WAIT_DOWNLOAD, nullptr, (Action)ChangeAssetToNormal }, // download
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },                 // download_finished
    },
    {
        // STATUS_WAIT_DOWNLOAD
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset },         // remote_changed
        { STATUS_STABLE, nullptr, (Action)CompensateSync },              // transfer_finished
        { STATUS_WAIT_UPLOAD, nullptr, (Action)ChangeAssetToNormal },    // upload
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },                  // upload_finished
        { STATUS_WAIT_DOWNLOAD, nullptr, (Action)STATUS_WAIT_DOWNLOAD }, // download
        { STATUS_NO_CHANGE, nullptr, (Action)Recover },                  // download_finished
    }
};

int32_t ObjectAssetMachine::DFAPostEvent(AssetEvent eventId, ChangedAssetInfo& changedAssetInfo, Asset& asset,
    const std::pair<std::string, Asset>& newAsset)
{
    if (eventId < 0 || eventId >= EVENT_BUTT) {
        return GeneralError::E_ERROR;
    }

    const DFAAction* action = &AssetDFA[changedAssetInfo.status][eventId];
    if (action->before != nullptr) {
        int32_t res = action->before(eventId, changedAssetInfo, asset, newAsset);
        if (res != GeneralError::E_OK) {
            return GeneralError::E_ERROR;
        }
    }
    if (action->next != STATUS_NO_CHANGE) {
        ZLOGI("status before:%{public}d, eventId: %{public}d, status after:%{public}d", changedAssetInfo.status,
            eventId, action->next);
        changedAssetInfo.status = static_cast<TransferStatus>(action->next);
    }
    if (action->after != nullptr) {
        int32_t res = action->after(eventId, changedAssetInfo, asset, newAsset);
        if (res != GeneralError::E_OK) {
            return GeneralError::E_ERROR;
        }
    }
    return GeneralError::E_OK;
}

static int32_t DoTransfer(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset)
{
    changedAsset.deviceId = newAsset.first;
    changedAsset.asset = newAsset.second;
    std::vector<Asset> assets{ changedAsset.asset };
    ObjectAssetLoader::GetInstance()->TransferAssetsAsync(changedAsset.storeInfo.user,
        changedAsset.storeInfo.bundleName, changedAsset.deviceId, assets, [&changedAsset](bool success) {
            if (success) {
                auto status = UpdateStore(changedAsset);
                if (status != E_OK) {
                    ZLOGE("UpdateStore error, error:%{public}d, assetName:%{public}s, store:%{public}s, "
                          "table:%{public}s",
                        status, changedAsset.asset.name.c_str(),
                        Anonymous::Change(changedAsset.bindInfo.storeName).c_str(),
                        changedAsset.bindInfo.tableName.c_str());
                }
            }
            ObjectAssetMachine::DFAPostEvent(TRANSFER_FINISHED, changedAsset, changedAsset.asset);
        });
    return E_OK;
}

static int32_t UpdateStore(ChangedAssetInfo& changedAsset)
{
    auto store = GetStore(changedAsset);
    if (store == nullptr) {
        ZLOGE("store null, storeId:%{public}s", Anonymous::Change(changedAsset.bindInfo.storeName).c_str());
        return E_ERROR;
    }

    VBuckets vBuckets = GetMigratedData(store, changedAsset.bindInfo, changedAsset.asset);
    if (vBuckets.empty()) {
        return E_OK;
    }
    return store->MergeMigratedData(changedAsset.bindInfo.tableName, std::move(vBuckets));
}

static VBuckets GetMigratedData(AutoCache::Store& store, AssetBindInfo& assetBindInfo, const Asset& newAsset)
{
    Values args;
    VBuckets vBuckets;
    auto sql = BuildSql(assetBindInfo, args);
    auto cursor = store->Query(assetBindInfo.tableName, sql, std::move(args));
    if (cursor == nullptr) {
        return vBuckets;
    }
    int32_t count = cursor->GetCount();
    if (count != 1) {
        return vBuckets;
    }
    vBuckets.reserve(count);
    auto err = cursor->MoveToFirst();
    while (err == E_OK && count > 0) {
        VBucket entry;
        err = cursor->GetRow(entry);
        if (err != E_OK) {
            return vBuckets;
        }
        MergeAssetData(entry, newAsset, assetBindInfo);
        vBuckets.emplace_back(std::move(entry));
        err = cursor->MoveToNext();
        count--;
    }
    return vBuckets;
}

static std::string BuildSql(const AssetBindInfo& bindInfo, Values& args)
{
    std::string sql;
    sql.append("SELECT ").append(bindInfo.field).append(" FROM ").append(bindInfo.tableName).append(" WHERE ");
    for (auto const& [key, value] : bindInfo.primaryKey) {
        sql.append(key).append(SQL_AND);
        args.emplace_back(value);
    }
    sql = sql.substr(0, sql.size() - AND_SIZE);
    return sql;
}

static void MergeAssetData(VBucket& record, const Asset& newAsset, const AssetBindInfo& assetBindInfo)
{
    for (auto const& [key, primary] : assetBindInfo.primaryKey) {
        record[key] = primary;
    }

    auto it = record.find(assetBindInfo.field);
    if (it == record.end()) {
        ZLOGD("Not find field:%{public}s in store", assetBindInfo.field.c_str());
        return;
    }

    auto& value = it->second;
    if (value.index() == TYPE_INDEX<std::monostate>) {
        Assets assets{ newAsset };
        value = assets;
        return;
    }
    if (value.index() == TYPE_INDEX<DistributedData::Asset>) {
        auto* asset = Traits::get_if<DistributedData::Asset>(&value);
        if (asset->name != newAsset.name) {
            ZLOGD("Asset not same, old uri: %{public}s, new uri: %{public}s", asset->uri.c_str(), newAsset.uri.c_str());
            return;
        }
    }

    if (value.index() == TYPE_INDEX<DistributedData::Assets>) {
        auto* assets = Traits::get_if<DistributedData::Assets>(&value);
        for (auto& asset : *assets) {
            if (asset.name == newAsset.name) {
                MergeAsset(asset, newAsset);
                return;
            }
        }
        assets->emplace_back(newAsset);
    }
}

static void MergeAsset(Asset& oldAsset, const Asset& newAsset)
{
    oldAsset.name = newAsset.name;
    oldAsset.uri = newAsset.uri;
    oldAsset.modifyTime = newAsset.modifyTime;
    oldAsset.createTime = newAsset.createTime;
    oldAsset.size = newAsset.size;
    oldAsset.hash = newAsset.hash;
    oldAsset.path = newAsset.path;
}

static AutoCache::Store GetStore(ChangedAssetInfo& changedAsset)
{
    StoreMetaData meta;
    meta.storeId = changedAsset.bindInfo.storeName;
    meta.bundleName = changedAsset.storeInfo.bundleName;
    meta.user = std::to_string(changedAsset.storeInfo.user);
    meta.instanceId = changedAsset.storeInfo.instanceId;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
        ZLOGE("meta empty, bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return nullptr;
    }
    return AutoCache::GetInstance().GetStore(meta, {});
}

static int32_t CompensateTransferring(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset)
{
    std::pair<std::string, Asset> newChangedAsset{ changedAsset.deviceId, changedAsset.asset };
    return ObjectAssetMachine::DFAPostEvent(REMOTE_CHANGED, changedAsset, changedAsset.asset, newChangedAsset);
}

static int32_t CompensateSync(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset)
{
    BindEvent::BindEventInfo bindEventInfo = MakeBindInfo(changedAsset);
    auto evt = std::make_unique<BindEvent>(BindEvent::COMPENSATE_SYNC, std::move(bindEventInfo));
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return E_OK;
}

static int32_t SaveNewAsset(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset)
{
    changedAsset.deviceId = newAsset.first;
    changedAsset.asset = newAsset.second;
    return E_OK;
}

static int32_t ChangeAssetToNormal(int32_t eventId, ChangedAssetInfo& changedAssetInfo, Asset& asset,
    const std::pair<std::string, Asset>& newAsset)
{
    asset.status = Asset::STATUS_NORMAL;
    return E_OK;
}

static int32_t Recover(int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset,
    const std::pair<std::string, Asset>& newAsset)
{
    ZLOGE("An abnormal event has occurred, eventId:%{public}d, status:%{public}d, assetName:%{public}s", eventId,
        changedAsset.status, changedAsset.asset.name.c_str());

    BindEvent::BindEventInfo bindEventInfo = MakeBindInfo(changedAsset);
    changedAsset.status = TransferStatus::STATUS_STABLE;
    auto evt = std::make_unique<BindEvent>(BindEvent::RECOVER_SYNC, std::move(bindEventInfo));
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return E_OK;
}

static BindEvent::BindEventInfo MakeBindInfo(ChangedAssetInfo& changedAsset)
{
    BindEvent::BindEventInfo bindEventInfo;
    bindEventInfo.bundleName = changedAsset.storeInfo.bundleName;
    bindEventInfo.user = changedAsset.storeInfo.user;
    bindEventInfo.tokenId = changedAsset.storeInfo.tokenId;
    bindEventInfo.instanceId = changedAsset.storeInfo.instanceId;
    bindEventInfo.storeName = changedAsset.bindInfo.storeName;
    bindEventInfo.tableName = changedAsset.bindInfo.tableName;
    bindEventInfo.filed = changedAsset.bindInfo.field;
    bindEventInfo.primaryKey = changedAsset.bindInfo.primaryKey;
    bindEventInfo.assetName = changedAsset.bindInfo.assetName;
    return bindEventInfo;
}

ObjectAssetMachine::ObjectAssetMachine() {}

} // namespace DistributedObject
} // namespace OHOS
