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
#define LOG_TAG "DataShareDbConfig"

#include "data_share_db_config.h"

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
std::pair<bool, DistributedData::StoreMetaData> DataShareDbConfig::QueryMetaData(
    const std::string &bundleName, const std::string &storeName, int32_t userId)
{
    DistributedData::StoreMetaData meta;
    meta.deviceId = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = std::to_string(userId);
    meta.bundleName = bundleName;
    meta.storeId = storeName;
    DistributedData::StoreMetaData metaData;
    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData, true);
    return std::make_pair(isCreated, metaData);
}

std::tuple<int, DistributedData::StoreMetaData, std::shared_ptr<DBDelegate>> DataShareDbConfig::GetDbConfig(
    const std::string &uri, bool hasExtension, const std::string &bundleName, const std::string &storeName,
    int32_t userId)
{
    auto [success, metaData] = QueryMetaData(bundleName, storeName, userId);
    if (!success) {
        if (!hasExtension) {
            ZLOGE("DB not exist, bundleName:%{public}s, storeName:%{public}s, userId:%{public}d",
                bundleName.c_str(), storeName.c_str(), userId);
            return std::make_tuple(NativeRdb::E_DB_NOT_EXIST, metaData, nullptr);
        }
        ExtensionConnectAdaptor::TryAndWait(uri, bundleName);
        auto [succ, meta] = QueryMetaData(bundleName, storeName, userId);
        if (!succ) {
            ZLOGE("Query metaData fail, bundleName:%{public}s, userId:%{public}d, uri:%{public}s",
                bundleName.c_str(), userId, URIUtils::Anonymous(uri).c_str());
            return std::make_tuple(NativeRdb::E_DB_NOT_EXIST, meta, nullptr);
        }
        metaData = std::move(meta);
    }
    auto dbDelegate = DBDelegate::Create(metaData);
    if (dbDelegate == nullptr) {
        ZLOGE("Create delegate fail, bundleName:%{public}s, userId:%{public}d, uri:%{public}s",
            bundleName.c_str(), userId, URIUtils::Anonymous(uri).c_str());
        return std::make_tuple(E_ERROR, metaData, nullptr);
    }
    return std::make_tuple(E_OK, metaData, dbDelegate);
}
} // namespace OHOS::DataShare
