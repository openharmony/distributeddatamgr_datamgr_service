/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_PERMISSION_PERMIT_DELEGATE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_PERMISSION_PERMIT_DELEGATE_H
#include <cstdint>
#include <map>
#include <string>
#include "concurrent_map.h"
#include "lru_bucket.h"
#include "metadata/store_meta_data.h"
#include "store_errno.h"
#include "types_export.h"
#include "visibility.h"

namespace OHOS::DistributedData {
using Status = OHOS::DistributedKv::Status;
using CheckParam = DistributedDB::PermissionCheckParam;
using CondParam = DistributedDB::PermissionConditionParam;
using ActiveParam = DistributedDB::ActivationCheckParam;

class PermitDelegate {
public:
    API_EXPORT static PermitDelegate &GetInstance();
    API_EXPORT void Init();
    API_EXPORT bool SyncActivate(const ActiveParam &param);
    API_EXPORT bool VerifyPermission(const CheckParam &param, uint8_t flag);
    API_EXPORT void DelCache(const std::string &key);

private:
    PermitDelegate();
    ~PermitDelegate();
    bool VerifyExtraCondition(const std::map<std::string, std::string> &cond) const;
    Status VerifyStrategy(const StoreMetaData &data, const std::string &rmdevId) const;
    Status LoadStoreMeta(const std::string &prefix, const CheckParam &param, StoreMetaData &data) const;
    std::map<std::string, std::string> GetExtraCondition(const CondParam &param);

    ConcurrentMap<std::string, std::string> appId2BundleNameMap_;
    LRUBucket<std::string, StoreMetaData> metaDataBucket_ {32};
    static constexpr const char *DEFAULT_USER = "0";
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_PERMISSION_PERMIT_DELEGATE_H
