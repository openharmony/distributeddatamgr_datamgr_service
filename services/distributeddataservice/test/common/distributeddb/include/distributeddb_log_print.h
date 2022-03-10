/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTED_DB_LOG_PRINT_H
#define DISTRIBUTED_DB_LOG_PRINT_H

#if defined _WIN32
    #ifndef RUNNING_ON_WIN
        #define RUNNING_ON_WIN
    #endif
#else
    #ifndef RUNNING_ON_LINUX
        #define RUNNING_ON_LINUX
    #endif
#endif

#include "hilog/log.h"

static constexpr OHOS::HiviewDFX::HiLogLabel LOG_LABEL = { LOG_CORE, 0xD001630, "DistributedDB[TEST]" };
#define MST_LOG(fmt, ...) \
    OHOS::HiviewDFX::HiLog::Info(LOG_LABEL, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#endif
