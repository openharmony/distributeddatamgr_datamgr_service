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
#ifndef ZLIB_COMPRESSION_H
#define ZLIB_COMPRESSION_H
#ifndef OMIT_ZLIB
#include <vector>
#include "data_compression.h"

namespace DistributedDB {
class ZlibCompression final : public DataCompression {
public:
    ZlibCompression();
    ~ZlibCompression() = default;

    int Compress(const std::vector<uint8_t> &srcData, std::vector<uint8_t> &destData) const override;
    int Uncompress(const std::vector<uint8_t> &srcData, std::vector<uint8_t> &destData, uint32_t destLen) const
        override;

protected:
    ZlibCompression(const ZlibCompression& compression) = delete;
    ZlibCompression& operator= (const ZlibCompression& compression) = delete;
};
}  // namespace DistributedDB
#endif // OMIT_ZLIB
#endif // ZLIB_COMPRESSION_H