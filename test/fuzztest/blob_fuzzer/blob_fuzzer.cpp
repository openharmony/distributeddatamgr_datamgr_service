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

#include "blob_fuzzer.h"

#include <cstdint>
#include <securec.h>
#include <vector>

#include "itypes_util.h"
#include "types.h"

using namespace OHOS::DistributedKv;
namespace OHOS {
Blob blobTest = "Test";
void BlobSelfOption(Blob blob)
{
    blob.Empty();
    blob.Size();
    blob.Data();
    blob.ToString();
    blob.RawSize();
}

void BlobEachOtherOption(Blob blob1, Blob blob2)
{
    blob1.Compare(blob2);
    MessageParcel parcel;
    Blob blobOut;
    blob1.Compare(blobOut);
    blob1.StartsWith(blob2);
}

void BlobOption(Blob blob)
{
    BlobSelfOption(blob);
    Blob blobTmp(blob);
    BlobEachOtherOption(blob, blobTmp);

    Blob blobPrefix = { "fuzz" };
    blobTmp = blobPrefix.ToString() + blob.ToString();
    if (blobPrefix[0] == blobTmp[0] && (!(blobPrefix == blobTmp))) {
        BlobEachOtherOption(blobTmp, blobPrefix);
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    std::string fuzzedString(reinterpret_cast<const char *>(data), size);
    std::vector<uint8_t> fuzzedVec(fuzzedString.begin(), fuzzedString.end());

    Blob blob1(reinterpret_cast<const char *>(data));
    blob1 = reinterpret_cast<const char *>(data);
    Blob blob2(fuzzedString);
    blob2 = fuzzedString;
    Blob blob3(fuzzedVec);
    Blob blob4(reinterpret_cast<const char *>(data), size);
    Blob blob5(blob4);
    Blob blob6(std::move(blob5));
    Blob blob7 = blob6;
    blob7 = Blob(blob6);
    OHOS::BlobOption(fuzzedString);
    int count = 10;
    uint8_t *writePtr = new uint8_t[count];
    Blob blob8(fuzzedString);
    blob8.WriteToBuffer(writePtr, count);
    const uint8_t *readPtr;
    blob8.ReadFromBuffer(readPtr, count);

    return 0;
}
