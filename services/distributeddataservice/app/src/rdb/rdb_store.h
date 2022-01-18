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

#ifndef DISTRIBUTEDDATASERVICE_RDB_STORE_H
#define DISTRIBUTEDDATASERVICE_RDB_STORE_H

#include "rdb_store_stub.h"
#include "rdb_parcel.h"

namespace OHOS::DistributedKv {
class RdbStore : public RdbStoreStub {
public:
    explicit RdbStore(const RdbStoreParam& param);
    
    RdbStore() = delete;
    RdbStore(const RdbStore&) = delete;
    RdbStore& operator=(const RdbStore&) = delete;
    
    virtual ~RdbStore();
    
    virtual int Init() = 0;
    
    bool operator==(const RdbStore& rhs) const;
    
    std::string GetBundleName() const;
    
    std::string GetAppId() const;
    
    std::string GetUserId() const;
    
    std::string GetStoreId() const;
    
    std::string GetIdentifier() const;
    
    std::string GetPath() const;
    
private:
    int type_;
    std::string bundleName_;
    std::string path_;
    std::string storeId_;
    std::string appId_;
    std::string userId_;
    std::string identifier_;
};
}
#endif
