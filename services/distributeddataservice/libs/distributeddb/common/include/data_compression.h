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
#ifndef DATA_COMPRESSION_H
#define DATA_COMPRESSION_H

#include <vector>
#include <set>
#include <map>

#include "types_export.h"

namespace DistributedDB {
class DataCompression {
public:
    static DataCompression *GetInstance(CompressAlgorithm algo);
    static void GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet);
    static int TransferCompressionAlgo(uint32_t compressAlgoType, CompressAlgorithm &algoType);

    virtual int Compress(const std::vector<uint8_t> &srcData, std::vector<uint8_t> &destData) const = 0;
    virtual int Uncompress(const std::vector<uint8_t> &srcData, std::vector<uint8_t> &destData, uint32_t destLen)
        const = 0;

protected:
    DataCompression() = default;
    virtual ~DataCompression() = default;
    DataCompression(const DataCompression& compression) = delete;
    DataCompression& operator= (const DataCompression& compression) = delete;

    static void Register(CompressAlgorithm algo, DataCompression *compression);

private:
    static std::map<CompressAlgorithm, DataCompression *> &GetCompressionAlgos();
    static std::map<uint32_t, CompressAlgorithm> &GetTransMap();
};
}  // namespace DistributedDB
#endif  // DATA_COMPRESSION_H