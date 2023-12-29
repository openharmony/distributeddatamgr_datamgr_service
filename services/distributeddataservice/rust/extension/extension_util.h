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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_EXTENSION_UTIL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_EXTENSION_UTIL_H

#include "basic_rust_types.h"
#include "cloud_ext_types.h"
#include "cloud_extension.h"
#include "cloud/cloud_info.h"
#include "cloud/schema_meta.h"
#include "error.h"
#include "error/general_error.h"
#include "store/general_value.h"

namespace OHOS::CloudData {
using DBMeta = DistributedData::Database;
using DBTable = DistributedData::Table;
using DBErr = DistributedData::GeneralError;
using DBValue = DistributedData::Value;
using DBBytes = DistributedData::Bytes;
using DBAsset = DistributedData::Asset;
using DBAssets = DistributedData::Assets;
using DBAssetStatus = DBAsset::Status;
using DBInfo = DistributedData::CloudInfo::AppInfo;
using DBVBucket = DistributedData::VBucket;
using DBVBuckets = DistributedData::VBuckets;
class ExtensionUtil final {
public:
    template<class T>
    inline static constexpr auto TYPE_INDEX = DistributedData::TYPE_INDEX<T>;
    static DBVBuckets ConvertBuckets(OhCloudExtVector *values);
    static DBVBucket ConvertBucket(OhCloudExtHashMap *value);
    static DBInfo ConvertAppInfo(OhCloudExtAppInfo *appInfo);
    static DBAsset ConvertAsset(OhCloudExtCloudAsset *asset);
    static DBValue ConvertValue(OhCloudExtValue *value);
    static DBValue ConvertValues(OhCloudExtValueBucket *bucket, const std::string &key);
    static DBErr ConvertStatus(int status);
    static DBAssets ConvertAssets(OhCloudExtVector *values);

    static std::pair<OhCloudExtHashMap *, size_t> Convert(const DBVBucket &bucket);
    static std::pair<OhCloudExtVector *, size_t> Convert(DBVBuckets &&buckets);
    static std::pair<OhCloudExtDatabase *, size_t> Convert(const DBMeta &dbMeta);
    static std::pair<OhCloudExtValue *, size_t> Convert(const DBValue &dbValue);
    static std::pair<OhCloudExtCloudAsset *, size_t> Convert(const DBAsset &dbAsset);
    static std::pair<OhCloudExtVector *, size_t> Convert(const DBAssets &dbAssets);
    static std::pair<OhCloudExtVector *, size_t> Convert(const DBTable &dbTable);
    static OhCloudExtAssetStatus ConvertAssetStatus(DBAssetStatus status);

private:
    static void ConvertAssetLeft(OhCloudExtCloudAsset *asset, DBAsset &dbAsset);
    static void DoConvertValue(void *content, unsigned int ctLen, OhCloudExtValueType type, DBValue &dbvalue);
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_EXTENSION_UTIL_H
