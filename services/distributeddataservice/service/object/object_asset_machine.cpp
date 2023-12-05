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

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "cloud/change_event.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "object_asset_loader.h"
#include "snapshot/snapshot_event.h"
#include "store/auto_cache.h"

namespace OHOS {
namespace DistributedObject {
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedRdb;
using Account = OHOS::DistributedKv::AccountDelegate;
using AccessTokenKit = Security::AccessToken::AccessTokenKit;
using OHOS::DistributedKv::AccountDelegate;
using namespace Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Account = OHOS::DistributedKv::AccountDelegate;
using AccessTokenKit = Security::AccessToken::AccessTokenKit;
using OHOS::DistributedKv::AccountDelegate;
using namespace Security::AccessToken;
constexpr static const char *SQL_AND = " = ? and ";
constexpr static const int32_t AND_SIZE = 5;

static int32_t DoTransfer(int32_t eventId, ChangedAssetInfo& changedAsset, std::pair<std::string, Asset>& newAsset);

static int32_t ChangeAssetToNormal(int32_t eventId, Asset& asset, void*);

static int32_t CompensateSync(int32_t eventId, ChangedAssetInfo& changedAsset, void*);

static int32_t CompensateTransferring(int32_t eventId, ChangedAssetInfo& changedAsset, const Asset& curAsset);

static int32_t SaveNewAsset(int32_t eventId, ChangedAssetInfo& changedAsset, std::pair<std::string, Asset>& newAsset);

static int32_t UpdateStore(ChangedAssetInfo& changedAsset);

static AutoCache::Store GetStore(ChangedAssetInfo& changedAsset);
static VBuckets GetMigratedData(AutoCache::Store& store, RdbBindInfo& rdbBindInfo, const Asset& newAsset);
static void MergeAssetData(VBucket& record, const Asset& newAsset, const RdbBindInfo& rdbBindInfo);
static void MergeAsset(Asset& oldAsset, const Asset& newAsset);
static std::string BuildSql(const RdbBindInfo& bindInfo, Values &args);

static const DFAAction AssetDFA[STATUS_BUTT][EVENT_BUTT] = {
    {
        // STATUS_STABLE
        { STATUS_TRANSFERRING, nullptr, (Action)DoTransfer }, // remote_changed
        { STATUS_NO_CHANGE, nullptr, nullptr },               // transfer_finished
        { STATUS_UPLOADING, nullptr, nullptr },               //upload
        { STATUS_NO_CHANGE, nullptr, nullptr },               // upload_finished
        { STATUS_DOWNLOADING, nullptr, nullptr },             // download
        { STATUS_NO_CHANGE, nullptr, nullptr },               // upload_finished
    },
    {
        // TRANSFERRING
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset },        // remote_changed
        { STATUS_STABLE, nullptr, nullptr },                            // transfer_finished
        { STATUS_WAIT_UPLOAD, nullptr, (Action)ChangeAssetToNormal },   //upload
        { STATUS_NO_CHANGE, nullptr, nullptr },                         // upload_finished
        { STATUS_WAIT_DOWNLOAD, nullptr, (Action)ChangeAssetToNormal }, // download
        { STATUS_NO_CHANGE, nullptr, nullptr },                         // upload_finished
    },
    {
        // DOWNLOADING
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset }, // remote_changed
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // transfer_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  //upload
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // upload_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // download
        { STATUS_STABLE, nullptr, nullptr },                     // download_finished
    },
    {
        // STATUS_UPLOADING
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset }, // remote_changed
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // transfer_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  //upload
        { STATUS_STABLE, nullptr, nullptr },                     // upload_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // download
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // download_finished
    },
    {
        // STATUS_WAIT_TRANSFER
        { STATUS_NO_CHANGE, nullptr, (Action)SaveNewAsset },            // remote_changed
        { STATUS_STABLE, nullptr, (Action)CompensateTransferring },     // transfer_finished
        { STATUS_WAIT_UPLOAD, nullptr, (Action)ChangeAssetToNormal },   //upload
        { STATUS_STABLE, nullptr, (Action)CompensateTransferring },     // upload_finished
        { STATUS_WAIT_DOWNLOAD, nullptr, (Action)ChangeAssetToNormal }, // download
        { STATUS_STABLE, nullptr, (Action)CompensateTransferring },     // download_finished
    },
    {
        // STATUS_WAIT_UPLOAD
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset }, // remote_changed
        { STATUS_STABLE, nullptr, (Action)CompensateSync },      // transfer_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  //upload
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // upload_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // download
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // download_finished
    },
    {
        // STATUS_WAIT_DOWNLOAD
        { STATUS_WAIT_TRANSFER, nullptr, (Action)SaveNewAsset }, // remote_changed
        { STATUS_STABLE, nullptr, (Action)CompensateSync },      // transfer_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  //upload
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // upload_finished
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // download
        { STATUS_NO_CHANGE, nullptr, nullptr },                  // download_finished
    }
};

int32_t ObjectAssetMachine::DFAPostEvent(AssetEvent eventId, TransferStatus& status, void* param, void* param2)
{
    if (eventId < 0 || eventId >= EVENT_BUTT) {
        return GeneralError::E_ERROR;
    }

    const DFAAction* action = &AssetDFA[status][eventId];
    if (action->before != nullptr) {
        int32_t res = action->before(eventId, param, param2);
        if (res != GeneralError::E_OK) {
            return GeneralError::E_ERROR;
        }
    }
    if (action->next != STATUS_NO_CHANGE) {
        ZLOGE("status before:%{public}d", status);
        status = static_cast<TransferStatus>(action->next);
        ZLOGE(" eventId: %{public}d, status after:%{public}d", eventId, status);
    } else {
        ZLOGE("status nochange: %{public}d", status);
    }
    if (action->after != nullptr) {
        int32_t res = action->after(eventId, param, param2);
        if (res != GeneralError::E_OK) {
            return GeneralError::E_ERROR;
        }
    }
    return GeneralError::E_OK;
}

static int32_t DoTransfer(int32_t eventId, ChangedAssetInfo& changedAsset, std::pair<std::string, Asset>& newAsset)
{
    changedAsset.deviceId = newAsset.first;
    changedAsset.asset = newAsset.second;
    ZLOGE("go to DownloadFile, deviceId:%{public}s, uri:%{public}s", changedAsset.deviceId.c_str(),
        changedAsset.asset.uri.c_str());

    ObjectAssetLoader::GetInstance()->DownLoad(changedAsset.storeInfo.user, changedAsset.storeInfo.bundleName,
        changedAsset.deviceId, changedAsset.asset, [&](bool success) {
            if (success) {
                auto status = UpdateStore(changedAsset);
                if (status != E_OK) {
                    ZLOGE("UpdateStore error, error:%{public}d", status);
                }
            }
            ObjectAssetMachine::DFAPostEvent(TRANSFER_FINISHED, changedAsset.status, (void*)&changedAsset, nullptr);
        });
    ZLOGE("DownloadFile end");
    return E_OK;
}

static int32_t UpdateStore(ChangedAssetInfo& changedAsset)
{
    auto store = GetStore(changedAsset);
    if (store == nullptr) {
        ZLOGE("store null, storeId:%{public}s", changedAsset.bindInfo.storeName.c_str());
        return E_ERROR;
    }

    VBuckets vBuckets = GetMigratedData(store, changedAsset.bindInfo, changedAsset.asset);
    return store->MergeMigratedData(changedAsset.bindInfo.tableName, std::move(vBuckets));
}

static VBuckets GetMigratedData(AutoCache::Store& store, RdbBindInfo& rdbBindInfo, const Asset& newAsset)
{
    Values args;
    VBuckets vBuckets;
    auto sql = BuildSql(rdbBindInfo, args);
    auto cursor = store->Query(rdbBindInfo.tableName, sql, std::move(args));
    if (cursor == nullptr) {
        ZLOGE("cursor is nullptr");
        return vBuckets;
    }
    int32_t count = cursor->GetCount();

    if (count != 1 && count != 0) {
        ZLOGE("Query Error, cursor count:%{public}d",count);
        return vBuckets;
    }
    if (count == 0) {
        VBucket entry;
        MergeAssetData(entry, newAsset, rdbBindInfo);
        vBuckets.emplace_back(std::move(entry));
        return vBuckets;
    }

    vBuckets.reserve(count);
    auto err = cursor->MoveToFirst();
    while (err ==E_OK && count> 0) {
        VBucket entry;
        err = cursor->GetRow(entry);
        if (err != E_OK) {
            break;
        }
        MergeAssetData(entry, newAsset, rdbBindInfo);
        vBuckets.emplace_back(std::move(entry));
        err = cursor->MoveToNext();
        count--;
    }
    return vBuckets;
}

static std::string BuildSql(const RdbBindInfo& bindInfo, Values &args) {

    std::string sql;
    sql.append("SELECT " ).append(bindInfo.field).append(" FROM ").append(bindInfo.tableName).append(" WHERE ");
    for (auto const&[key, value]: bindInfo.primaryKey) {
        sql.append(key).append(SQL_AND);
        args.emplace_back(value);
    }
    sql = sql.substr(0, sql.size() - AND_SIZE);
    return sql;
}

static void MergeAssetData(VBucket& record, const Asset& newAsset, const RdbBindInfo& rdbBindInfo)
{
    for (auto const&[key, primary]: rdbBindInfo.primaryKey) {
        record[key] = primary;
    }

    auto it = record.find(rdbBindInfo.field);
    if (it == record.end() ) {
        ZLOGE("Error, Not find field:%{public}s in store", rdbBindInfo.field.c_str());
        return;
    }

    auto &value = it->second;
    if (value.index() == TYPE_INDEX<std::monostate>) {
        Assets assets {newAsset};
        return;
    }
    if (value.index() == TYPE_INDEX<DistributedData::Asset>) {
        auto* asset = Traits::get_if<DistributedData::Asset>(&value);
        if (asset->name != newAsset.name) {
            ZLOGE("Asset not same, old uri: %{public}s, new uri: %{public}s",asset->uri.c_str(), newAsset.uri.c_str());
        }
        MergeAsset(*asset, newAsset);
        return;
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

static void MergeAsset(Asset& oldAsset, const Asset& newAsset) {
    oldAsset.name = newAsset.name;
    oldAsset.uri = newAsset.uri;
    oldAsset.modifyTime = newAsset.modifyTime;
    oldAsset.createTime = newAsset.createTime;
    oldAsset.size = newAsset.size;
    oldAsset.hash = newAsset.hash;
}

static AutoCache::Store GetStore(ChangedAssetInfo& changedAsset)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    HapTokenInfo tokenInfo;
    auto status = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (status != RET_SUCCESS) {
        ZLOGE("token:0x%{public}x, result:%{public}d", tokenId, status);
        return nullptr;
    }

    StoreMetaData meta;
    meta.storeId = changedAsset.bindInfo.storeName;
    meta.bundleName = tokenInfo.bundleName;
    meta.user = std::to_string(tokenInfo.userID);
    meta.instanceId = tokenInfo.instIndex;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
        ZLOGE("meta empty, bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
              meta.GetStoreAlias().c_str());
        return nullptr;
    }
    return AutoCache::GetInstance().GetStore(meta, {});
}

static int32_t CompensateTransferring(int32_t eventId, ChangedAssetInfo& changedAsset, const Asset& curAsset)
{
    if (curAsset.hash == changedAsset.asset.hash) {
        return E_OK;
    }

    std::pair<std::string, Asset> newChangedAsset{ changedAsset.deviceId, changedAsset.asset };
    return ObjectAssetMachine::DFAPostEvent(REMOTE_CHANGED, changedAsset.status, (void*)&changedAsset,
        (void*)&newChangedAsset);
}

static int32_t CompensateSync(int32_t eventId, ChangedAssetInfo& changedAsset, void*)
{
    ZLOGE("DoCloudSync");

    SnapshotEvent::SnapshotEventInfo bindEventInfo;
    bindEventInfo.bundleName = changedAsset.storeInfo.bundleName;
    bindEventInfo.user = changedAsset.storeInfo.user;
    bindEventInfo.instanceId = changedAsset.storeInfo.instanceId;
    bindEventInfo.storeName = changedAsset.bindInfo.storeName;
    bindEventInfo.tableName = changedAsset.bindInfo.tableName;
    bindEventInfo.filed = changedAsset.bindInfo.field;
    bindEventInfo.primaryKey = changedAsset.bindInfo.primaryKey;
    bindEventInfo.assetName = changedAsset.bindInfo.assetName;

    auto evt = std::make_unique<SnapshotEvent>(SnapshotEvent::COMPENSATE_SYNC, bindEventInfo);
    EventCenter::GetInstance().PostEvent(std::move(evt));
    // rdb接受以后进行补偿同步
    return E_OK;
}

static int32_t SaveNewAsset(int32_t eventId, ChangedAssetInfo& changedAsset, std::pair<std::string, Asset>& newAsset)
{
    changedAsset.deviceId = newAsset.first;
    changedAsset.asset = newAsset.second;
    return E_OK;
}

static int32_t ChangeAssetToNormal(int32_t eventId, Asset& asset, void*)
{
    asset.status = Asset::STATUS_NORMAL;
    return E_OK;
}

ObjectAssetMachine::ObjectAssetMachine() {}

} // namespace DistributedObject
} // namespace OHOS
