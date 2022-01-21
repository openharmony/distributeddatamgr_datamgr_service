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

#ifndef DISTRIBUTED_RDB_SYNCER_IMPL_H
#define DISTRIBUTED_RDB_SYNCER_IMPL_H

#include "rdb_syncer_stub.h"
#include "rdb_types.h"

namespace OHOS::DistributedRdb {
class RdbSyncerImpl : public RdbSyncerStub {
public:
    explicit RdbSyncerImpl(const RdbSyncerParam& param);
    
    RdbSyncerImpl() = delete;
    RdbSyncerImpl(const RdbSyncerImpl&) = delete;
    RdbSyncerImpl& operator=(const RdbSyncerImpl&) = delete;
    
    virtual ~RdbSyncerImpl();
    
    virtual int Init() = 0;
    
    bool operator==(const RdbSyncerImpl& rhs) const;
    
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
