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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_EXPORTER_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_EXPORTER_H
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "metadata/store_meta_data.h"
namespace OHOS::DistributedKv {
class KVDBExporter {
public:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using DBPassword = DistributedDB::CipherPassword;
    KVDBExporter() noexcept;
    ~KVDBExporter() {}
    static void Exporter(const StoreMetaData &meta, const std::string &backupPath, bool &result);
private:
    static KVDBExporter instance_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_EXPORTER_H
