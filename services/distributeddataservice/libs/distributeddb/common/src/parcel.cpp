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

#include "parcel.h"

#include <climits>

#include "endian_convert.h"
#include "securec.h"
#include "macro_utils.h"
#include "log_print.h"
#include "db_errno.h"
#include "db_constant.h"

namespace DistributedDB {
Parcel::Parcel(uint8_t *inBuf, uint32_t len)
    : buf_(inBuf),
      bufPtr_(inBuf),
      totalLen_(len)
{
    if (inBuf == nullptr || len == 0) {
        isError_ = true;
    }
}

Parcel::~Parcel()
{
    buf_ = nullptr;
    bufPtr_ = nullptr;
}

bool Parcel::IsError() const
{
    return isError_;
}

int Parcel::WriteBool(bool data)
{
    const uint32_t boolLen = GetBoolLen();
    if (isError_ || parcelLen_ + boolLen > totalLen_) {
        isError_ = true;
        return -E_PARSE_FAIL;
    }
    errno_t errCode = memcpy_s(bufPtr_, totalLen_ - parcelLen_, &data, sizeof(bool));
    if (errCode != EOK) {
        isError_ = true;
        return -E_SECUREC_ERROR;
    }
    bufPtr_ += boolLen;
    parcelLen_ += boolLen;
    return E_OK;
}

uint32_t Parcel::ReadBool(bool &val)
{
    const uint32_t boolLen = GetBoolLen();
    if (isError_ || parcelLen_ + boolLen > totalLen_) {
        isError_ = true;
        return -E_PARSE_FAIL;
    }
    val = *(reinterpret_cast<bool *>(bufPtr_));
    bufPtr_ += boolLen;
    parcelLen_ += boolLen;
    return boolLen;
}

int Parcel::WriteInt(int32_t data)
{
    return WriteInteger(data);
}

uint32_t Parcel::ReadInt(int32_t &val)
{
    return ReadInteger(val);
}


int Parcel::WriteDouble(double data)
{
    double inData = HostToNet(data);
    if (isError_ || parcelLen_ + sizeof(double) > totalLen_) {
        isError_ = true;
        return -E_PARSE_FAIL;
    }
    errno_t errCode = memcpy_s(bufPtr_, totalLen_ - parcelLen_, &inData, sizeof(double));
    if (errCode != EOK) {
        isError_ = true;
        return -E_SECUREC_ERROR;
    }
    bufPtr_ += sizeof(double);
    parcelLen_ += sizeof(double);
    return E_OK;
}

uint32_t Parcel::ReadDouble(double &val)
{
    if (isError_ || bufPtr_ == nullptr || parcelLen_ + sizeof(double) > totalLen_) {
        isError_ = true;
        return 0;
    }
    val = *(reinterpret_cast<double *>(bufPtr_));
    bufPtr_ += sizeof(double);
    parcelLen_ += sizeof(double);
    val = NetToHost(val);
    return sizeof(double);
}

int Parcel::WriteInt64(int64_t data)
{
    return WriteInteger(data);
}

uint32_t Parcel::ReadInt64(int64_t &val)
{
    return ReadInteger(val);
}

int Parcel::WriteUInt32(uint32_t data)
{
    return WriteInteger(data);
}

uint32_t Parcel::ReadUInt32(uint32_t &val)
{
    return ReadInteger(val);
}

int Parcel::WriteUInt64(uint64_t data)
{
    return WriteInteger(data);
}

uint32_t Parcel::ReadUInt64(uint64_t &val)
{
    return ReadInteger(val);
}

int Parcel::WriteVectorChar(const std::vector<uint8_t>& data)
{
    return WriteVector<uint8_t>(data);
}

uint32_t Parcel::ReadVectorChar(std::vector<uint8_t>& val)
{
    return ReadVector<uint8_t>(val);
}

int Parcel::WriteString(const std::string &inVal)
{
    if (inVal.size() > INT32_MAX) {
        LOGE("[WriteString] Invalid string, size:%zu.", inVal.size());
        isError_ = true;
        return -E_PARSE_FAIL;
    }
    if (IsError()) {
        return -E_PARSE_FAIL;
    }
    uint32_t len = inVal.size();
    uint64_t stepLen = sizeof(uint32_t) + static_cast<uint64_t>(inVal.size());
    len = HostToNet(len);
    if (stepLen > INT32_MAX || parcelLen_ + BYTE_8_ALIGN(stepLen) > totalLen_) {
        LOGE("[WriteString] stepLen:%llu, totalLen:%llu, parcelLen:%llu", stepLen, totalLen_, parcelLen_);
        isError_ = true;
        return -E_PARSE_FAIL;
    }
    errno_t errCode = memcpy_s(bufPtr_, totalLen_ - parcelLen_, &len, sizeof(uint32_t));
    if (errCode != EOK) {
        LOGE("[WriteString] bufPtr:%d, totalLen:%llu, parcelLen:%llu", bufPtr_ != nullptr, totalLen_, parcelLen_);
        isError_ = true;
        return -E_SECUREC_ERROR;
    }
    bufPtr_ += sizeof(uint32_t);
    if (inVal.size() == 0) {
        bufPtr_ += BYTE_8_ALIGN(stepLen) - stepLen;
        parcelLen_ += BYTE_8_ALIGN(stepLen);
        return errCode;
    }
    errCode = memcpy_s(bufPtr_, totalLen_ - parcelLen_ - sizeof(uint32_t), inVal.c_str(), inVal.size());
    if (errCode != EOK) {
        LOGE("[WriteString] totalLen:%llu, parcelLen:%llu, inVal.size:%zu.",
            totalLen_, parcelLen_, inVal.size());
        isError_ = true;
        return -E_SECUREC_ERROR;
    }
    bufPtr_ += inVal.size();
    bufPtr_ += BYTE_8_ALIGN(stepLen) - stepLen;
    parcelLen_ += BYTE_8_ALIGN(stepLen);
    return E_OK;
}

uint32_t Parcel::ReadString(std::string &outVal)
{
    if (IsError()) {
        return 0;
    }
    if (bufPtr_ == nullptr || parcelLen_ + sizeof(uint32_t) > totalLen_) {
        LOGE("[ReadString] bufPtr:%d, totalLen:%llu, parcelLen:%llu", bufPtr_ != nullptr, totalLen_, parcelLen_);
        isError_ = true;
        return 0;
    }
    uint32_t len = *(reinterpret_cast<uint32_t *>(bufPtr_));
    len = NetToHost(len);
    uint64_t stepLen = static_cast<uint64_t>(len) + sizeof(uint32_t);
    if (stepLen > INT32_MAX || parcelLen_ + BYTE_8_ALIGN(stepLen) > totalLen_) {
        LOGE("[ReadString] stepLen:%llu, totalLen:%llu, parcelLen:%llu", stepLen, totalLen_, parcelLen_);
        isError_ = true;
        return 0;
    }
    outVal.resize(len);
    outVal.assign(bufPtr_ + sizeof(uint32_t), bufPtr_ + stepLen);
    bufPtr_ += BYTE_8_ALIGN(stepLen);
    parcelLen_ += BYTE_8_ALIGN(stepLen);
    stepLen = BYTE_8_ALIGN(stepLen);
    return static_cast<uint32_t>(stepLen);
}

bool Parcel::IsContinueRead()
{
    return (parcelLen_ < totalLen_);
}

#ifndef OMIT_MULTI_VER
int Parcel::WriteMultiVerCommit(const MultiVerCommitNode &commit)
{
    int errCode = WriteVectorChar(commit.commitId);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write commitId err!");
        isError_ = true;
        return errCode;
    }
    errCode = WriteVectorChar(commit.leftParent);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write leftParent err!");
        return errCode;
    }
    errCode = WriteVectorChar(commit.rightParent);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write rightParent err!");
        return errCode;
    }
    errCode = WriteUInt64(commit.timestamp);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write timestamp err!");
        return errCode;
    }
    errCode = WriteUInt64(commit.version);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write version err!");
        return errCode;
    }
    errCode = WriteUInt64(commit.isLocal);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write isLocal err!");
        return errCode;
    }
    errCode = WriteString(commit.deviceInfo);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write deviceInfo err!");
    }
    return errCode;
}

uint32_t Parcel::ReadMultiVerCommit(MultiVerCommitNode &commit)
{
    if (isError_) {
        return 0;
    }
    uint64_t len = ReadVectorChar(commit.commitId);
    len += ReadVectorChar(commit.leftParent);
    len += ReadVectorChar(commit.rightParent);
    len += ReadUInt64(commit.timestamp);
    len += ReadUInt64(commit.version);
    len += ReadUInt64(commit.isLocal);
    len += ReadString(commit.deviceInfo);
    if (isError_ || len > INT32_MAX) {
        isError_ = true;
        return 0;
    }
    return static_cast<uint32_t>(len);
}
int Parcel::WriteMultiVerCommits(const std::vector<MultiVerCommitNode> &commits)
{
    uint64_t len = commits.size();
    int errCode = WriteUInt64(len);
    if (errCode != E_OK) {
        LOGE("Parcel::WriteMultiVerCommit write len err!");
        isError_ = true;
        return errCode;
    }
    for (auto &iter : commits) {
        errCode = WriteVectorChar(iter.commitId);
        if (errCode != E_OK) {
            LOGE("Parcel::WriteMultiVerCommit write commitId err!");
            return errCode;
        }
        errCode = WriteVectorChar(iter.leftParent);
        if (errCode != E_OK) {
            LOGE("Parcel::WriteMultiVerCommit write leftParent err!");
            return errCode;
        }
        errCode = WriteVectorChar(iter.rightParent);
        if (errCode != E_OK) {
            LOGE("Parcel::WriteMultiVerCommit write rightParent err!");
            return errCode;
        }
        errCode = WriteUInt64(iter.timestamp);
        if (errCode != E_OK) {
            LOGE("Parcel::WriteMultiVerCommit write timestamp err!");
            return errCode;
        }
        errCode = WriteUInt64(iter.version);
        if (errCode != E_OK) {
            LOGE("Parcel::WriteMultiVerCommit write version err!");
            return errCode;
        }
        errCode = WriteUInt64(iter.isLocal);
        if (errCode != E_OK) {
            LOGE("Parcel::WriteMultiVerCommit write isLocal err!");
            return errCode;
        }
        errCode = WriteString(iter.deviceInfo);
        if (errCode != E_OK) {
            LOGE("Parcel::WriteMultiVerCommit write deviceInfo err!");
            return errCode;
        }
        EightByteAlign();
    }
    EightByteAlign();
    return errCode;
}

uint32_t Parcel::ReadMultiVerCommits(std::vector<MultiVerCommitNode> &commits)
{
    uint64_t len = 0;
    uint64_t size = 0;
    len += ReadUInt64(size);
    if (isError_) {
        return 0;
    }
    if (size > DBConstant::MAX_COMMIT_SIZE) {
        isError_ = true;
        LOGE("Parcel::ReadMultiVerCommits commits size too large: %llu", size);
        return 0;
    }
    for (uint64_t i = 0; i < size; i++) {
        MultiVerCommitNode commit;
        len += ReadVectorChar(commit.commitId);
        len += ReadVectorChar(commit.leftParent);
        len += ReadVectorChar(commit.rightParent);
        len += ReadUInt64(commit.timestamp);
        len += ReadUInt64(commit.version);
        len += ReadUInt64(commit.isLocal);
        len += ReadString(commit.deviceInfo);
        commits.push_back(commit);
        EightByteAlign();
        len = BYTE_8_ALIGN(len);
        if (isError_ || len > INT32_MAX) {
            isError_ = true;
            return 0;
        }
    }
    len = BYTE_8_ALIGN(len);

    return static_cast<uint32_t>(len);
}
#endif

int Parcel::WriteBlob(const char *buffer, uint32_t bufLen)
{
    if (buffer == nullptr) {
        LOGE("[WriteBlob] Invalid buffer.");
        isError_ = true;
        return -E_INVALID_ARGS;
    }
    if (IsError()) {
        return -E_PARSE_FAIL;
    }
    if (parcelLen_ + bufLen > totalLen_) {
        LOGE("[WriteBlob] bufLen:%u, totalLen:%llu, parcelLen:%llu", bufLen, totalLen_, parcelLen_);
        isError_ = true;
        return -E_PARSE_FAIL;
    }
    uint32_t leftLen = static_cast<uint32_t>(totalLen_ - parcelLen_);
    int errCode = memcpy_s(bufPtr_, leftLen, buffer, bufLen);
    if (errCode != EOK) {
        LOGE("[WriteBlob] bufPtr:%d, leftLen:%u, buffer:%p, bufLen:%u", bufPtr_ != nullptr, leftLen, buffer, bufLen);
        isError_ = true;
        return -E_SECUREC_ERROR;
    }
    uint32_t length = (BYTE_8_ALIGN(bufLen) < leftLen) ? BYTE_8_ALIGN(bufLen) : leftLen;
    bufPtr_ += length;
    parcelLen_ += length;
    return E_OK;
}
uint32_t Parcel::ReadBlob(char *buffer, uint32_t bufLen)
{
    if (buffer == nullptr) {
        LOGE("[ReadBlob] Invalid buffer.");
        isError_ = true;
        return 0;
    }
    if (IsError()) {
        return 0;
    }
    uint32_t leftLen = static_cast<uint32_t>(totalLen_ - parcelLen_);
    if (parcelLen_ + bufLen > totalLen_) {
        LOGE("[ReadBlob] bufLen:%u, totalLen:%llu, parcelLen:%llu", bufLen, totalLen_, parcelLen_);
        isError_ = true;
        return 0;
    }
    int errCode = memcpy_s(buffer, bufLen, bufPtr_, bufLen);
    if (errCode != EOK) {
        LOGE("[ReadBlob] bufPtr:%d, buffer:%p, bufLen:%u", bufPtr_ != nullptr, buffer, bufLen);
        isError_ = true;
        return 0;
    }
    uint32_t length = (BYTE_8_ALIGN(bufLen) < leftLen) ? BYTE_8_ALIGN(bufLen) : leftLen;
    bufPtr_ += length;
    parcelLen_ += length;
    return length;
}

uint32_t Parcel::GetBoolLen()
{
    return GetEightByteAlign(sizeof(bool));
}

uint32_t Parcel::GetIntLen()
{
    return sizeof(int32_t);
}

uint32_t Parcel::GetUInt32Len()
{
    return sizeof(uint32_t);
}

uint32_t Parcel::GetUInt64Len()
{
    return sizeof(uint64_t);
}

uint32_t Parcel::GetInt64Len()
{
    return sizeof(int64_t);
}

uint32_t Parcel::GetDoubleLen()
{
    return sizeof(double);
}

uint32_t Parcel::GetVectorCharLen(const std::vector<uint8_t> &data)
{
    return GetVectorLen<uint8_t>(data);
}

uint32_t Parcel::GetStringLen(const std::string &data)
{
    if (data.size() > INT32_MAX) {
        return 0;
    }
    uint64_t len = sizeof(uint32_t) + static_cast<uint64_t>(data.size());
    len = BYTE_8_ALIGN(len);
    if (len > INT32_MAX) {
        return 0;
    }
    return static_cast<uint32_t>(len);
}

#ifndef OMIT_MULTI_VER
uint32_t Parcel::GetMultiVerCommitLen(const MultiVerCommitNode &commit)
{
    uint64_t len = GetVectorCharLen(commit.commitId);
    len += GetVectorCharLen(commit.leftParent);
    len += GetVectorCharLen(commit.rightParent);
    len += GetUInt64Len();
    len += GetUInt64Len();
    len += GetUInt64Len();
    len += GetStringLen(commit.deviceInfo);
    if (len > INT32_MAX) {
        return 0;
    }
    return static_cast<uint32_t>(len);
}

uint32_t Parcel::GetMultiVerCommitsLen(const std::vector<MultiVerCommitNode> &commits)
{
    uint64_t len = GetUInt64Len();
    for (auto &iter : commits) {
        len += GetVectorCharLen(iter.commitId);
        len += GetVectorCharLen(iter.leftParent);
        len += GetVectorCharLen(iter.rightParent);
        len += GetUInt64Len();
        len += GetUInt64Len();
        len += GetUInt64Len();
        len += GetStringLen(iter.deviceInfo);
        len = BYTE_8_ALIGN(len);
        if (len > INT32_MAX) {
            return 0;
        }
    }
    len = BYTE_8_ALIGN(len);
    if (len > INT32_MAX) {
        return 0;
    }
    return static_cast<uint32_t>(len);
}
#endif

void Parcel::EightByteAlign()
{
    bufPtr_ += BYTE_8_ALIGN(parcelLen_) - parcelLen_;
    parcelLen_ = BYTE_8_ALIGN(parcelLen_);
}

uint32_t Parcel::GetEightByteAlign(uint32_t len)
{
    return BYTE_8_ALIGN(len);
}

uint32_t Parcel::GetAppendedLen()
{
    // 8 is 8-byte-align max append len, there are 2 8-byte-align totally
    return sizeof(uint32_t) + sizeof(uint32_t) + 8 * 2;
}
} // namespace DistributedDB
