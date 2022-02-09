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

#ifndef DISTRIBUTED_RDB_STORE_FACTORY_H
#define DISTRIBUTED_RDB_STORE_FACTORY_H

#include <functional>
#include <memory>
#include <map>
#include <sys/types.h>
namespace OHOS::DistributedRdb {
class RdbSyncerImpl;
struct RdbSyncerParam;
class RdbSyncerFactory {
public:
    using Creator = std::function<RdbSyncerImpl* (const RdbSyncerParam&, pid_t)>;
    
    static RdbSyncerFactory& GetInstance();
    
    void Register(int type, const Creator& creator);
    
    void UnRegister(int type);
    
    RdbSyncerImpl* CreateSyncer(const RdbSyncerParam& param, pid_t uid);

private:
    std::map<int, Creator> creators_;
};

template <typename T>
class RdbSyncerCreator {
public:
    RdbSyncerImpl* operator()(const RdbSyncerParam& param, pid_t uid)
    {
        return static_cast<RdbSyncerImpl*>(new(std::nothrow) T(param, uid));
    }
};

template <typename T, int type>
class RdbSyncerRegistration {
public:
    RdbSyncerRegistration();
    
    ~RdbSyncerRegistration();
    
    RdbSyncerRegistration(const RdbSyncerRegistration&) = delete;
    RdbSyncerRegistration(RdbSyncerRegistration&&) = delete;
    RdbSyncerRegistration& operator=(const RdbSyncerRegistration&) = delete;
    RdbSyncerRegistration& operator=(RdbSyncerRegistration&&) = delete;
};

template <typename T, int type>
RdbSyncerRegistration<T, type>::RdbSyncerRegistration()
{
    RdbSyncerFactory::GetInstance().Register(type, RdbSyncerCreator<T>());
}

template <typename T, int type>
RdbSyncerRegistration<T, type>::~RdbSyncerRegistration()
{
    RdbSyncerFactory::GetInstance().UnRegister(type);
}
}
#endif
