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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_SCHEMA_CONFIG_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_SCHEMA_CONFIG_H
#include <string>
#include <vector>
#include "bundlemgr/bundle_mgr_proxy.h"
#include "metadata/store_meta_data.h"
#include "serializable/serializable.h"
#include "cloud/schema_meta.h"
namespace OHOS::DistributedRdb {
using RdbSchema = DistributedData::Database;
using DbSchema = DistributedData::SchemaMeta;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
class RdbSchemaConfig {
public:
    static bool GetDistributedSchema(const StoreMetaData &meta, RdbSchema &rdbSchema);

private:
    static bool InitBundleInfo(const std::string &bundleName, int32_t userId, OHOS::AppExecFwk::BundleInfo &bundleInfo);
    static bool GetSchemaFromHap(
        const OHOS::AppExecFwk::BundleInfo &bundleInfo, const std::string &storeName, RdbSchema &rdbSchema);
};
}  // namespace OHOS::DistributedRdb
#endif  // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_SCHEMA_CONFIG_H