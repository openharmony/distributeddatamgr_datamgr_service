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

#include "zlib_compression.h"
#ifndef OMIT_ZLIB
#include <zlib.h>

#include "db_constant.h"
#include "db_errno.h"
#include "log_print.h"
#include "types_export.h"

namespace DistributedDB {
static ZlibCompression g_zlibInstance;

ZlibCompression::ZlibCompression()
{
    DataCompression::Register(CompressAlgorithm::ZLIB, this);
}

int ZlibCompression::Compress(const std::vector<uint8_t> &srcData, std::vector<uint8_t> &destData) const
{
    auto srcLen = srcData.size();
    auto destLen = compressBound(srcLen);
    if (srcLen > DBConstant::MAX_SYNC_BLOCK_SIZE || destLen > DBConstant::MAX_SYNC_BLOCK_SIZE) {
        LOGE("Too long to compress, srcLen:%zu, destLen:%lu.", srcLen, destLen);
        return -E_INVALID_ARGS;
    }

    // Alloc memory.
    destData.resize(destLen);

    // Compress.
    int errCode = compress(destData.data(), &destLen, srcData.data(), srcLen);
    if (errCode != Z_OK) {
        LOGE("Compress parcel failed, errCode = %d", errCode);
        return -E_SYSTEM_API_FAIL;
    }

    destData.resize(destLen);
    destData.shrink_to_fit();
    return E_OK;
}

int ZlibCompression::Uncompress(const std::vector<uint8_t> &srcData, std::vector<uint8_t> &destData,
    uint32_t destLen) const
{
    auto srcLen = srcData.size();
    if (srcLen > DBConstant::MAX_SYNC_BLOCK_SIZE || destLen > DBConstant::MAX_SYNC_BLOCK_SIZE) {
        LOGE("Too long to uncompress, srcLen:%zu, destLen:%" PRIu32 ".", srcLen, destLen);
        return -E_INVALID_ARGS;
    }

    // Alloc dest memory.
    destData.resize(destLen);

    // Uncompress.
    uLongf destDataLen = destLen;
    int errCode = uncompress(destData.data(), &destDataLen, srcData.data(), srcData.size());
    if (errCode != Z_OK) {
        LOGE("Uncompress failed, errCode = %d", errCode);
        return -E_SYSTEM_API_FAIL;
    }

    destData.resize(destDataLen);
    destData.shrink_to_fit();
    return E_OK;
}
}  // namespace DistributedDB
#endif // OMIT_ZLIB
