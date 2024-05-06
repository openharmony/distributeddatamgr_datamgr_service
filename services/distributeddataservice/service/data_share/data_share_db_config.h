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
#ifndef DATASHARESERVICE_DATA_SHARE_DB_CONFIG_H
#define DATASHARESERVICE_DATA_SHARE_DB_CONFIG_H

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>

#include "db_delegate.h"
#include "extension_ability_info.h"
#include "metadata/store_meta_data.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
class DataShareDbConfig {
public:
    std::tuple<int, DistributedData::StoreMetaData, std::shared_ptr<DBDelegate>> GetDbConfig(
        const std::string &uri, bool hasExtension, const std::string &bundleName,
        const std::string &storeName, int32_t userId);
private:
    static std::pair<bool, DistributedData::StoreMetaData> QueryMetaData(const std::string &bundleName,
        const std::string &storeName, int32_t userId);
};
} // namespace OHOS::DataShare
#endif
