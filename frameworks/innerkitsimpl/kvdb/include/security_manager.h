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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SECURITY_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SECURITY_MANAGER_H
#include "types.h"
#include "types_export.h"
namespace OHOS::DistributedKv {
class API_EXPORT SecurityManager {
public:
    using DBPassword = DistributedDB::CipherPassword;
    static SecurityManager &GetInstance();
    DBPassword GetDBPassword(const StoreId &storeId, const std::string &path, bool encrypt);
    void DelDBPassword(const StoreId &storeId, const std::string &path);

private:
    static constexpr const char *ROOT_KEY_ALIAS = "distributed_db_root_key";
    static constexpr const char *STRATEGY_META_PREFIX = "StrategyMetaData";
    static constexpr const char *CAPABILITY_ENABLED = "capabilityEnabled";
    static constexpr const char *CAPABILITY_RANGE = "capabilityRange";
    static constexpr const char *LOCAL_LABEL = "localLabel";
    static constexpr const char *REMOTE_LABEL = "remoteLabel";
    static constexpr const char *HARMONY_APP = "harmony";
    static constexpr const char *HKS_BLOB_TYPE_NONCE = "Z5s0Bo571KoqwIi6";
    static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata";
    static constexpr int KEY_SIZE = 32;
    static constexpr int HOURS_PER_YEAR = (24 * 365);

    static std::vector<uint8_t> Random(int32_t len);
    std::vector<uint8_t> LoadRandomKey(const StoreId &storeId, const std::string &path);
    bool SaveRandomKey(const StoreId &storeId, const std::string &path, const std::vector<uint8_t> &key);
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SECURITY_MANAGER_H
