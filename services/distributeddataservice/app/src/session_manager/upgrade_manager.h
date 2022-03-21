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

#ifndef DISTRIBUTEDDATAMGR_UPGRADE_MANAGER_H
#define DISTRIBUTEDDATAMGR_UPGRADE_MANAGER_H
#include <string>

#include "auth/auth_delegate.h"
#include "concurrent_map.h"
#include "kvstore_meta_manager.h"
#include "metadata/capability_meta_data.h"
#include "types.h"
namespace OHOS::DistributedData {
using DistributedDB::KvStoreNbDelegate;
using OHOS::DistributedKv::KvStoreTuple;

class UpgradeManager {
public:
    static UpgradeManager &GetInstance();
    void Init();
    CapMetaData GetCapability(const std::string &deviceId, bool &status);
    static void SetCompatibleIdentifyByType(
        KvStoreNbDelegate *storeDelegate, const KvStoreTuple &tuple, AUTH_GROUP_TYPE groupType);
    static std::string GetIdentifierByType(int32_t groupType, bool &isSuccess);

private:
    bool InitLocalCapability();
    ConcurrentMap<std::string, CapMetaData> capabilityMap_ {};
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_UPGRADE_MANAGER_H
