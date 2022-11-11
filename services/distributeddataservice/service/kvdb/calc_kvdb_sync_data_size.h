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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CALC_KVDB_SYNC_DATA_SIZE_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CALC_KVDB_SYNC_DATA_SIZE_H

#include "metadata/store_meta_data.h"
#include "store_types.h"
#include "visibility.h"
namespace OHOS::DistributedKv {
using namespace DistributedDB;
using namespace OHOS::DistributedData;
class API_EXPORT CalcKvSyncDataSize final {
public:
    static uint32_t CalcSyncDataSize(const StoreMetaData &data, DBStatus &status);
};
}
#endif //DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CALC_KVDB_SYNC_DATA_SIZE_H
