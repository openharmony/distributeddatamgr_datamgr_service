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
#ifndef DATASHARESERVICE_DATA_SHARE_DB_DELEGATE_H
#define DATASHARESERVICE_DATA_SHARE_DB_DELEGATE_H

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>

#include "db_delegate.h"
#include "extension_ability_info.h"
#include "metadata/store_meta_data.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
class DataShareDbDelegate {
public:
    DataShareDbDelegate(const std::string &bundleName, const std::string &storeName,
        int32_t userId) : bundleName_(bundleName), storeName_(storeName), userId_(userId) {}
    ~DataShareDbDelegate() = default;
    struct DbInfo {
        std::string dataDir;
        std::string secretKey;
        int version = -1;
        bool isEncrypt;
    };
    std::tuple<int, DbInfo, std::shared_ptr<DBDelegate>> GetDbInfo(const std::string uri,
        bool haveDataShareExtension);
private:
    std::pair<bool, DistributedData::StoreMetaData> QueryMetaData();
    std::string bundleName_;
    std::string storeName_;
    int32_t userId_;
    DbInfo dbInfo_;
};
} // namespace OHOS::DataShare
#endif
