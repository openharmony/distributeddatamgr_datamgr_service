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

#define LOG_TAG "ObjectSnapshot"
#include "object_snapshot.h"
#include "store/general_store.h"
#include "snapshot/snapshot_event.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
namespace OHOS::DistributedObject {

int32_t ObjectSnapshot::Upload(Asset& asset)
{
    if (!IsBindAsset(asset)) {
        return 0;
    }
    return assetMachine_->DFAPostEvent(UPLOAD, changedAssets_[asset.uri].status, (void*)&asset, nullptr);

}

bool ObjectSnapshot::IsBindAsset(const Asset& asset)
{
    auto it = changedAssets_.find(asset.uri);
    if (it != changedAssets_.end()) {
        return true;
    }
    return false;
}

int32_t ObjectSnapshot::Download(Asset& asset)
{
    if (!IsBindAsset(asset)) {
        return 0;
    }
    return assetMachine_->DFAPostEvent(DOWNLOAD, changedAssets_[asset.uri].status, (void*)&asset, nullptr);
}

TransferStatus ObjectSnapshot::GetAssetStatus(Asset& asset)
{
    if (!IsBindAsset(asset)) {
        return STATUS_BUTT;
    }
    return changedAssets_[asset.uri].status;

}

int32_t ObjectSnapshot::FinishUploading(Asset& asset)
{
    if (!IsBindAsset(asset)) {
        return E_OK;
    }
    return assetMachine_->DFAPostEvent(UPLOAD_FINISHED, changedAssets_[asset.uri].status, (void*)&changedAssets_[asset.uri], nullptr);
}

int32_t ObjectSnapshot::FinishDownloading(Asset& asset)
{
    if (!IsBindAsset(asset)) {
        return E_OK;
    }
    return assetMachine_->DFAPostEvent(DOWNLOAD_FINISHED, changedAssets_[asset.uri].status, (void*)&changedAssets_[asset.uri], nullptr);
}

int32_t ObjectSnapshot::OnDataChanged(Asset& asset, const std::string& deviceId)
{
    if (!IsBindAsset(asset)) {
        return E_OK;
    }
    std::pair<std::string, Asset> newAsset {deviceId, asset};
    return assetMachine_->DFAPostEvent(REMOTE_CHANGED,changedAssets_[asset.uri].status, (void*)&changedAssets_[asset.uri], (void*)&newAsset);
}

int32_t ObjectSnapshot::BindAsset(const Asset& asset, const RdbBindInfo& bindInfo, const StoreInfo& storeInfo)
{
    if (IsBindAsset(asset)) {
        ZLOGE("Asset is bound.asset.uri:%{public}s :", asset.uri.c_str());
        return E_OK;
    }
    changedAssets_[asset.uri] = ChangedAssetInfo(asset, bindInfo, storeInfo);

    return E_OK;
}

int32_t ObjectSnapshot::FinishTransferring(Asset& asset)
{
    if (!IsBindAsset(asset)) {
        return E_OK;
    }
    return assetMachine_->DFAPostEvent(TRANSFER_FINISHED, changedAssets_[asset.uri].status, (void*)&changedAssets_[asset.uri], nullptr);
}
ObjectSnapshot::~ObjectSnapshot() {}

ObjectSnapshot::ObjectSnapshot() {
    assetMachine_ = std::make_shared<ObjectAssetMachine>();
}
}