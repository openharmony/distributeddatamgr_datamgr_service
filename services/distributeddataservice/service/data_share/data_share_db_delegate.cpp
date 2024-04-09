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
#define LOG_TAG "DataShareDbDelegate"

#include "data_share_db_delegate.h"

#include <tuple>
#include <utility>

#include "datashare_errno.h"
#include "device_manager_adapter.h"
#include "extension_connect_adaptor.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
std::pair<bool, DistributedData::StoreMetaData> DataShareDbDelegate::QueryMetaData()
{
    DistributedData::StoreMetaData metaData;
    DistributedData::StoreMetaData meta;
    meta.deviceId = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = std::to_string(userId_);
    meta.bundleName = bundleName_;
    meta.storeId = storeName_;
    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData, true);
    if (!isCreated) {
        ZLOGE("DB not exist, bundleName:%{public}s, storeName:%{public}s, userId:%{public}d",
            bundleName_.c_str(), storeName_.c_str(), userId_);
    }
    return std::make_pair(isCreated, metaData);
}

std::tuple<int, DataShareDbDelegate::DbInfo, std::shared_ptr<DBDelegate>> DataShareDbDelegate::GetDbInfo(
    const std::string uri, bool haveDataShareExtension)
{
    auto [success, metaData] = QueryMetaData();
    if (!success) {
        ExtensionConnectAdaptor::TryAndWait(uri, bundleName_, haveDataShareExtension);
        auto [success, meta] = QueryMetaData();
        if (!success) {
            ZLOGE("Query metaData fail, bundleName:%{public}s, userId:%{public}d, uri:%{public}s",
                bundleName_.c_str(), userId_, DistributedData::Anonymous::Anonymity(uri).c_str());
            return std::make_tuple(NativeRdb::E_DB_NOT_EXIST, dbInfo_, nullptr);
        }
        metaData = std::move(meta);
    }
    dbInfo_.dataDir = std::move(metaData.dataDir);
    dbInfo_.isEncrypt = std::move(metaData.isEncrypt);
    dbInfo_.secretKey = metaData.isEncrypt ? metaData.GetSecretKey() : "";
    auto dbDelegate = DBDelegate::Create(dbInfo_.dataDir, dbInfo_.version,
        true, dbInfo_.isEncrypt, dbInfo_.secretKey);
    if (dbDelegate == nullptr) {
        ZLOGE("DbDelegate fail, bundleName:%{public}s,tokenId:0x%{public}x, uri:%{public}s",
            bundleName_.c_str(), userId_, DistributedData::Anonymous::Anonymity(uri).c_str());
        return std::make_tuple(E_ERROR, dbInfo_, nullptr);
    }
    return std::make_tuple(E_OK, dbInfo_, dbDelegate);
}
} // namespace OHOS::DataShare
