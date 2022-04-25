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

#ifndef QUERY_FUZZER_H
#define QUERY_FUZZER_H

#define FUZZ_PROJECT_NAME "Query_fuzzer"

#include <cstdint>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

uint16_t U16_AT(const uint8_t * const  &ptr)
{
    // 8 - 0
    return (ptr[0] << 8) | ptr[1];
}

uint32_t U32_AT(const uint8_t * const &ptr)
{
    //24 - 16 - 8 - 0
    return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

#endif //QUERY_FUZZER_H
