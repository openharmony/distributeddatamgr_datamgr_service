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

#ifndef DISTRIBUTEDDATAMGR_ENDIAN_CONVERTER_H
#define DISTRIBUTEDDATAMGR_ENDIAN_CONVERTER_H

#include <arpa/inet.h>
#include <asm/byteorder.h>

static inline uint64_t htonll(const uint64_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    constexpr const uint32_t UINT32_SIZE = 32;
    constexpr const uint64_t UINT64_MASK_HIGH = 0xFFFFFFFF00000000U;
    constexpr const uint64_t UINT64_MASK_LOW = 0x00000000FFFFFFFFU;
    return ((((uint64_t)htonl(value)) << UINT32_SIZE) & UINT64_MASK_HIGH)
           | (htonl(value >> UINT32_SIZE) & UINT64_MASK_LOW);
#endif
    return value;
}

static inline uint64_t ntohll(const uint64_t value)
{
    return htonll(value);
}
#endif // DISTRIBUTEDDATAMGR_ENDIAN_CONVERTER_H
