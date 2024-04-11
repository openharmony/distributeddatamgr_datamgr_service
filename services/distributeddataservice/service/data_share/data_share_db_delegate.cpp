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
bool DataShareDbDelegate::QueryMetaData()
{
    DistributedData::StoreMetaData meta;
    meta.deviceId = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = std::to_string(userId_);
    meta.bundleName = bundleName_;
    meta.storeId = storeName_;
    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData_, true);
    if (!isCreated) {
        ZLOGE("DB not exist, bundleName:%{public}s, storeName:%{public}s, userId:%{public}d",
            bundleName_.c_str(), storeName_.c_str(), userId_);
    }
    return isCreated;
}

std::tuple<int, DistributedData::StoreMetaData, std::shared_ptr<DBDelegate>> DataShareDbDelegate::GetDbInfo(
    const std::string uri, bool hasExtension)
{
    auto success = QueryMetaData();
    if (!success) {
        if (!hasExtension) {
            return std::make_tuple(NativeRdb::E_DB_NOT_EXIST, metaData_, nullptr);
        }
        ExtensionConnectAdaptor::TryAndWait(uri, bundleName_);
        success = QueryMetaData();
        if (!success) {
            ZLOGE("Query metaData fail, bundleName:%{public}s, userId:%{public}d, uri:%{public}s",
                bundleName_.c_str(), userId_, URIUtils::Anonymous(uri).c_str());
            return std::make_tuple(NativeRdb::E_DB_NOT_EXIST, metaData_, nullptr);
        }
    }
    // dbInfo_.dataDir = std::move(metaData.dataDir);
    // dbInfo_.isEncrypt = std::move(metaData.isEncrypt);
    // dbInfo_.secretKey = metaData.isEncrypt ? metaData.GetSecretKey() : "";
    auto dbDelegate = DBDelegate::Create(metaData_.dataDir, metaData_.version,
        true, metaData_.isEncrypt, metaData_.GetSecretKey());
    if (dbDelegate == nullptr) {
        ZLOGE("Create delegate fail, bundleName:%{public}s,tokenId:0x%{public}x, uri:%{public}s",
            bundleName_.c_str(), userId_, URIUtils::Anonymous(uri).c_str());
        return std::make_tuple(E_ERROR, metaData_, nullptr);
    }
    return std::make_tuple(E_OK, metaData_, dbDelegate);
}
} // namespace OHOS::DataShare
