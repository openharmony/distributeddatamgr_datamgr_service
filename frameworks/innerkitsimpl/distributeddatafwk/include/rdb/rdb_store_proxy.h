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

#ifndef DISTRIBUTEDDATAFWK_IRDB_STORE_PROXY_H
#define DISTRIBUTEDDATAFWK_IRDB_STORE_PROXY_H

#include <iremote_proxy.h>
#include "irdb_store.h"

namespace OHOS::DistributedKv {
class RdbStoreProxy : public IRemoteProxy<IRdbStore> {
public:
    explicit RdbStoreProxy(const sptr<IRemoteObject>& object);
    
    int SetDistributedTables(const std::vector<std::string>& tables) override;

private:
    static inline BrokerDelegator<RdbStoreProxy> delegator_;
};
}
#endif
