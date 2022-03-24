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

#include "data_compression.h"
#include <map>
#include "db_errno.h"

namespace DistributedDB {
void DataCompression::GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet)
{
    algorithmSet.clear();
    for (const auto &item : GetCompressionAlgos()) {
        algorithmSet.insert(item.first);
    }
}

int DataCompression::TransferCompressionAlgo(uint32_t compressAlgoType, CompressAlgorithm &algoType)
{
    auto iter = GetTransMap().find(compressAlgoType);
    if (iter == GetTransMap().end()) {
        return -E_INVALID_ARGS;
    }
    algoType = iter->second;
    return E_OK;
}

DataCompression *DataCompression::GetInstance(CompressAlgorithm algo)
{
    auto iter = GetCompressionAlgos().find(algo);
    if (iter == GetCompressionAlgos().end()) {
        return nullptr;
    }
    return iter->second;
}

// All supported compression algorithm should call this function to register their instance.
void DataCompression::Register(CompressAlgorithm algo, DataCompression *compressionPtr)
{
    if (GetInstance(algo) != nullptr) {
        return;
    }
    GetCompressionAlgos().insert({algo, compressionPtr});
    GetTransMap().insert({static_cast<uint32_t>(algo), algo});
}

std::map<CompressAlgorithm, DataCompression *> &DataCompression::GetCompressionAlgos()
{
    static std::map<CompressAlgorithm, DataCompression *> compressionAlgos;
    return compressionAlgos;
}

std::map<uint32_t, CompressAlgorithm> &DataCompression::GetTransMap()
{
    static std::map<uint32_t, CompressAlgorithm> transferMap;
    return transferMap;
}
}  // namespace DistributedDB
