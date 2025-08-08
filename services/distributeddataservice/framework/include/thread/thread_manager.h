/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_THREAD_MANAGER_H
#define DISTRIBUTEDDATAMGR_THREAD_MANAGER_H

#include "stdint.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class ThreadManager {
public:
    API_EXPORT static ThreadManager &GetInstance();
    API_EXPORT void Initialize(uint32_t minThreadNum, uint32_t maxThreadNum, uint32_t ipcThreadNum);
    API_EXPORT uint32_t GetMinThreadNum();
    API_EXPORT uint32_t GetMaxThreadNum();
    API_EXPORT uint32_t GetIpcThreadNum();

private:
    ThreadManager();
    uint32_t minThreadNum_ = 4;  // default min executor pool thread num
    uint32_t maxThreadNum_ = 12; // default max executor pool thread num
    uint32_t ipcThreadNum_ = 16; // normal ipc num
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_THREAD_MANAGER_H