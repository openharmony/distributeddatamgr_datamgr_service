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

#ifndef DISTRIBUTED_KV_BLOB_H
#define DISTRIBUTED_KV_BLOB_H

#include <string>
#include <vector>
#include "parcel.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
// note: Blob derives from Parcelable, so hiding inner a interface using blob is not possible unless Parcelable
// declared its interface as visible.
class Blob : public virtual Parcelable {
public:
    API_EXPORT Blob();

    API_EXPORT ~Blob() = default;

    // copy constructor for Blob.
    API_EXPORT Blob(const Blob &blob);
    API_EXPORT Blob &operator=(const Blob &blob);

    // move constructor for Blob.
    API_EXPORT Blob(Blob &&blob);
    API_EXPORT Blob &operator=(Blob &&blob);

    // construct a Blob use std::string.
    API_EXPORT Blob(const std::string &str);

    // construct a Blob use char pointer and len.
    API_EXPORT Blob(const char *str, size_t n);

    // construct a Blob use char pointer.
    API_EXPORT Blob(const char *str);

    // construct a Blob use std::vector<uint8_t>
    API_EXPORT Blob(const std::vector<uint8_t> &bytes);

    // construct a Blob use std::vector<uint8_t>
    API_EXPORT Blob(std::vector<uint8_t> &&bytes);

    // Return a reference to the data of the blob.
    API_EXPORT const std::vector<uint8_t> &Data() const;

    API_EXPORT operator const std::vector<uint8_t> &() const;

    API_EXPORT operator std::vector<uint8_t> &&() noexcept;

    // Return the length (in bytes) of the referenced data
    API_EXPORT size_t Size() const;

    // Return the occupied length when write this blob to rawdata
    int RawSize() const;

    // Return true if the length of the referenced data is zero
    API_EXPORT bool Empty() const;

    // Return the the byte in the referenced data.
    // REQUIRES: n < size()
    API_EXPORT uint8_t operator[](size_t n) const;

    API_EXPORT bool operator==(const Blob &) const;

    // Change this blob to refer to an empty array
    API_EXPORT void Clear();

    // change vector<uint8_t> to std::string
    API_EXPORT std::string ToString() const;

    // comparison.  Returns value:
    //   <  0 if "*this" <  "blob",
    //   == 0 if "*this" == "blob",
    //   >  0 if "*this" >  "blob"
    API_EXPORT int Compare(const Blob &blob) const;

    // Return true if "blob" is a prefix of "*this"
    API_EXPORT bool StartsWith(const Blob &blob) const;

    // Write a parcelable object to the given parcel.
    // The object position is saved into Parcel if set asRemote_ to
    // true, and this intends to use in kernel data transaction.
    // Returns true being written on success or false if any error occur.
    API_EXPORT bool Marshalling(Parcel &parcel) const override;

    // get data from the given parcel into this parcelable object.
    API_EXPORT static Blob *Unmarshalling(Parcel &parcel);

    /* write blob size and data to memory buffer. return error when bufferLeftSize not enough. */
    bool WriteToBuffer(uint8_t *&cursorPtr, int &bufferLeftSize) const;

    /* read a blob from memory buffer. */
    bool ReadFromBuffer(const uint8_t *&cursorPtr, int &bufferLeftSize);

private:
    std::vector<uint8_t> blob_;
};
}  // namespace DistributedKv
}  // namespace OHOS

#endif  // DISTRIBUTED_KV_BLOB_H
