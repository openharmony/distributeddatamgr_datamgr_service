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

#include "rdb_manager.h"
#include "rdb_manager_impl.h"

namespace OHOS::DistributedRdb {
std::shared_ptr<RdbSyncer> RdbManager::GetRdbSyncer(const RdbSyncerParam &param)
{
    return RdbManagerImpl::GetInstance().GetRdbSyncer(param);
}

int RdbManager::RegisterRdbServiceDeathObserver(const std::string &storeName, const std::function<void()>& observer)
{
    return RdbManagerImpl::GetInstance().RegisterRdbServiceDeathObserver(storeName, observer);
}

int RdbManager::UnRegisterRdbServiceDeathObserver(const std::string &storeName)
{
    return RdbManagerImpl::GetInstance().UnRegisterRdbServiceDeathObserver(storeName);
}
}