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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SYSTEM_API_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SYSTEM_API_H
#include "iprocess_system_api_adapter.h"
namespace OHOS::DistributedKv {
class SystemApi : public DistributedDB::IProcessSystemApiAdapter {
public:
    using AccessEventHanle = DistributedDB::OnAccessControlledEvent;
    using DBStatus = DistributedDB::DBStatus;
    using DBOption = DistributedDB::SecurityOption;
    SystemApi();
    ~SystemApi();
    DBStatus RegOnAccessControlledEvent(const AccessEventHanle &callback) override;
    bool IsAccessControlled() const override;
    DBStatus SetSecurityOption(const std::string &filePath, const DBOption &option) override;
    DBStatus GetSecurityOption(const std::string &filePath, DBOption &option) const override;
    bool CheckDeviceSecurityAbility(const std::string &devId, const DBOption &option) const override;
};
}
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SYSTEM_API_H
