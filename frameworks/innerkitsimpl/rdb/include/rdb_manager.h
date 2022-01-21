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

#ifndef DISTRIBUTED_RDB_MANAGER_H
#define DISTRIBUTED_RDB_MANAGER_H

#include <functional>
#include <string>
#include <memory>


namespace OHOS::DistributedRdb {
struct RdbSyncerParam;
class RdbSyncer;

class RdbManager {
public:
    static std::shared_ptr<RdbSyncer> GetRdbSyncer(const RdbSyncerParam& param);
    
    static int RegisterRdbServiceDeathObserver(const std::string &storeName, const std::function<void()>& observer);
    
    static int UnRegisterRdbServiceDeathObserver(const std::string &storeName);
    
};
}
#endif //DISTRIBUTED_RDB_MANAGER_H
