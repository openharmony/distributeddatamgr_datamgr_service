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

#ifndef DISTRIBUTEDDATAMGR_DATA_BUFFER_H
#define DISTRIBUTEDDATAMGR_DATA_BUFFER_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace AppDistributedKv {
struct HeaderInfo {
    uint32_t type;
    uint32_t length;
    uint32_t version;
    uint32_t sequence;
} __attribute__((packed));

class DataBuffer {
public:
    DataBuffer();

    ~DataBuffer();

    bool Init(size_t size);

    const char *GetBufPtr() const;

    size_t GetBufSize() const;

    void SetBufUsed(size_t used);

    size_t GetBufUsed() const;

    // header length
    static const int HEADER_LEN = sizeof(HeaderInfo);

// max size for transferring data using pipe is 5M
    static const size_t MAX_DATA_LEN;

// 5M; max size for transfer using pipe (12 is header length, rest is data wait for transferring)
    static const int MAX_TRANSFER_SIZE;

    static const uint32_t VERSION;

    static const uint32_t TYPE;

private:
    char *buf_;
    size_t size_;
    size_t used_;
    static int sequence_;
};

union Head {
    HeaderInfo headerInfo;
    uint8_t headArray[DataBuffer::HEADER_LEN];
} __attribute__((packed));
} // namespace AppDistributedKv
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_DATA_BUFFER_H
