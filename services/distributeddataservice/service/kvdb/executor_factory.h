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
#ifndef DISTRIBUTEDDATAMGR_DATAMGR_EXECUTOR_FACTORY_H
#define DISTRIBUTEDDATAMGR_DATAMGR_EXECUTOR_FACTORY_H

#include "kv_store_thread_pool.h"
namespace OHOS::DistributedData {
using OHOS::DistributedKv::KvStoreTask;
using OHOS::DistributedKv::KvStoreThreadPool;
class ExecutorFactory {
public:
    API_EXPORT static ExecutorFactory &GetInstance();
    API_EXPORT bool Execute(KvStoreTask &&task);

private:
    ExecutorFactory();
    ~ExecutorFactory();

    static constexpr int POOL_SIZE = 4;

    std::shared_ptr<KvStoreThreadPool> threadPool_;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_DATAMGR_EXECUTOR_FACTORY_H
