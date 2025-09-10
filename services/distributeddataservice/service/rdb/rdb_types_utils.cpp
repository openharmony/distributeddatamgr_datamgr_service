/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "rdb_types_utils.h"
namespace OHOS::DistributedRdb {
std::vector<std::string> RdbTypesUtils::GetSearchableTables(const RdbChangedData &changedData)
{
    std::vector<std::string> tables;
    for (auto &[key, value] : changedData.tableData) {
        if (value.isTrackedDataChange) {
            tables.push_back(key);
        }
    }
    return tables;
}

std::vector<std::string> RdbTypesUtils::GetP2PTables(const RdbChangedData &changedData)
{
    std::vector<std::string> tables;
    for (auto &[key, value] : changedData.tableData) {
        if (value.isP2pSyncDataChange) {
            tables.push_back(key);
        }
    }
    return tables;
}
std::vector<DistributedData::Reference> RdbTypesUtils::Convert(const std::vector<Reference> &references)
{
    std::vector<DistributedData::Reference> relationships;
    for (const auto &reference : references) {
        DistributedData::Reference relationship = { reference.sourceTable, reference.targetTable, reference.refFields };
        relationships.emplace_back(relationship);
    }
    return relationships;
}
} // namespace OHOS::DistributedRdb