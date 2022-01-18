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

#ifndef RDB_STORE_FACTORY_H
#define RDB_STORE_FACTORY_H

#include <memory>
#include "rdb_store.h"

namespace OHOS::DistributedKv {
class RdbStoreFactory {
public:
    using Creator = std::function<RdbStore* (const RdbStoreParam&)>;
    
    static void Initialize();
    
    static int RegisterCreator(int type, Creator& creator);
    
    static RdbStore* CreateStore(const RdbStoreParam& param);
    
private:
    static Creator creators_[RDB_DISTRIBUTED_TYPE_MAX];
};
}
#endif
