/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#define LOG_TAG "KvStoreThread"

#include "kv_store_thread_pool_impl.h"
#include "log_print.h"
#include "pthread.h"

namespace OHOS {
namespace DistributedKv {
KvStoreThread::KvStoreThread(KvStoreThreadPool *threadPool, const std::string &name)
{
    realThread_ = std::thread([this, threadPool, name]() {
        // this makes me unconfortable: this lambda capture 'this' by reference, and right after this call this object
        // is move-constructed, so when we call this in Run(), we are actually refer to the old object. we can still
        // use all its non-virtual function, but all arguments and virtual-function are not available.
        int32_t ret = pthread_setname_np(pthread_self(), name.c_str());
        if (ret != 0) {
            ZLOGE("Failed to set thread name:%{public}s, ret:%{public}d.", name.c_str(), ret);
        }
        Run(threadPool);
    });
}

void KvStoreThread::Run(KvStoreThreadPool *pool)
{
    if (pool == nullptr) {
        ZLOGW("input param is null.");
        return;
    }

    auto impl = reinterpret_cast<KvStoreThreadPoolImpl *>(pool);
    while (impl->IsRunning()) {
        impl->ScheduleTask()();
    }
    ZLOGW("stop");
}

void KvStoreThread::Join()
{
    realThread_.join();
}

KvStoreThread::~KvStoreThread()
{}
} // namespace DistributedKv
} // namespace OHOS
