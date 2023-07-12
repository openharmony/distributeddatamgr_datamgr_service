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

#ifndef UDMF_DATA_MANAGER_H
#define UDMF_DATA_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "error_code.h"
#include "store_cache.h"
#include "unified_data.h"
#include "unified_types.h"

namespace OHOS {
namespace UDMF {
class DataManager {
public:
    virtual ~DataManager();

    static DataManager &GetInstance();

    int32_t SaveData(CustomOption &option, UnifiedData &unifiedData, std::string &key);
    int32_t RetrieveData(const QueryOption &query, UnifiedData &unifiedData);
    int32_t RetrieveBatchData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet);
    int32_t UpdateData(const QueryOption &query, UnifiedData &unifiedData);
    int32_t DeleteData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet);
    int32_t GetSummary(const QueryOption &query, Summary &summary);
    int32_t AddPrivilege(const QueryOption &query, const Privilege &privilege);
    int32_t Sync(const QueryOption &query, const std::vector<std::string> &devices);

private:
    DataManager();
    int32_t QueryDataCommon(const QueryOption &query, std::vector<UnifiedData> &dataSet, std::shared_ptr<Store> &store);
    StoreCache storeCache_;
    std::map<std::string, std::string> authorizationMap_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_DATA_MANAGER_H
