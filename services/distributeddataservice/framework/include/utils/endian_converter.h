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

#include <endian.h>

inline uint16_t htobe(uint16_t value)
{
    return htobe16(value);
}

inline uint16_t betoh(uint16_t value)
{
    return be16toh(value);
}

inline uint32_t htobe(uint32_t value)
{
    return htobe32(value);
}

inline uint32_t betoh(uint32_t value)
{
    return be32toh(value);
}

inline uint64_t htobe(uint64_t value)
{
    return htobe64(value);
}

inline uint64_t betoh(uint64_t value)
{
    return be64toh(value);
}
#endif // DISTRIBUTEDDATAMGR_ENDIAN_CONVERTER_H
