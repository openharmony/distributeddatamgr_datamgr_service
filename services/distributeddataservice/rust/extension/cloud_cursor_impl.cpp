/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "ExtensionCloudCursorImpl"
#include "cloud_cursor_impl.h"
#include <set>
#include "extension_util.h"
#include "log_print.h"

namespace OHOS::CloudData {
static std::map<std::string, std::string> EXTEND_TO_KEYS = {
    { DistributedData::SchemaMeta::DELETE_FIELD, "operation" },
    { DistributedData::SchemaMeta::GID_FIELD, "id" },
    { DistributedData::SchemaMeta::CREATE_FIELD, "createTime" },
    { DistributedData::SchemaMeta::MODIFY_FIELD, "modifyTime" }
};

static std::set<std::string> EXTEND_KEYS = {
    "operation",
    "id",
    "createTime",
    "modifyTime"
};

CloudCursorImpl::CloudCursorImpl(OhCloudExtCloudDbData *cloudData) : cloudData_(cloudData)
{
    OhCloudExtCloudDbDataGetValues(cloudData_, &values_);
    if (values_ == nullptr) {
        return;
    }
    OhCloudExtVectorGetLength(values_, reinterpret_cast<unsigned int *>(&valuesLen_));
    if (valuesLen_ > 0) {
        void *values = nullptr;
        size_t valueLen = 0;
        auto status = OhCloudExtVectorGet(values_, 0, &values, reinterpret_cast<unsigned int *>(&valueLen));
        if (status == ERRNO_SUCCESS && values != nullptr) {
            OhCloudExtValueBucket *vBucket = reinterpret_cast<OhCloudExtValueBucket *>(values);
            auto data = GetData(vBucket);
            for (auto &[key, value] : data) {
                names_.push_back(key);
            }
            OhCloudExtValueBucketFree(vBucket);
        }
    }
    bool hasMore = false;
    OhCloudExtCloudDbDataGetHasMore(cloudData_, &hasMore);
    finished_ = !hasMore;
    unsigned char *cursor = nullptr;
    size_t cursorLen = 0;
    auto status = OhCloudExtCloudDbDataGetNextCursor(
        cloudData_, &cursor, reinterpret_cast<unsigned int *>(&cursorLen));
    if (status == ERRNO_SUCCESS && cursor != nullptr) {
        cursor_ = std::string(reinterpret_cast<char *>(cursor), cursorLen);
    }
}

CloudCursorImpl::~CloudCursorImpl()
{
    if (cloudData_ != nullptr) {
        OhCloudExtCloudDbDataFree(cloudData_);
        cloudData_ = nullptr;
    }
    if (values_ != nullptr) {
        OhCloudExtVectorFree(values_);
        values_ = nullptr;
    }
}

int32_t CloudCursorImpl::GetColumnNames(std::vector<std::string> &names) const
{
    if (values_ == nullptr) {
        return DBErr::E_ALREADY_CLOSED;
    }
    names = names_;
    return DBErr::E_OK;
}

int32_t CloudCursorImpl::GetColumnName(int32_t col, std::string &name) const
{
    if (names_.size() <= static_cast<size_t>(col)) {
        return DBErr::E_INVALID_ARGS;
    }
    name = names_[col];
    return DBErr::E_OK;
}

int32_t CloudCursorImpl::GetColumnType(int32_t col) const
{
    return DBErr::E_NOT_SUPPORT;
}

int32_t CloudCursorImpl::GetCount() const
{
    return static_cast<int32_t>(valuesLen_);
}

int32_t CloudCursorImpl::MoveToFirst()
{
    if (values_ == nullptr) {
        return DBErr::E_ALREADY_CLOSED;
    }
    if (index_ != INVALID_INDEX || valuesLen_ == 0) {
        return DBErr::E_ALREADY_CONSUMED;
    }
    index_ = 0;
    consumed_ = false;
    return DBErr::E_OK;
}

int32_t CloudCursorImpl::MoveToNext()
{
    if (values_ == nullptr) {
        return DBErr::E_ALREADY_CLOSED;
    }
    if (index_ >= valuesLen_) {
        return DBErr::E_ALREADY_CONSUMED;
    }
    if (index_ == INVALID_INDEX) {
        index_ = 0;
    } else {
        ++index_;
    }
    consumed_ = false;
    return DBErr::E_OK;
}

int32_t CloudCursorImpl::MoveToPrev()
{
    return DBErr::E_NOT_SUPPORT;
}

int32_t CloudCursorImpl::GetEntry(DBVBucket &entry)
{
    if (index_ == INVALID_INDEX) {
        return DBErr::E_NOT_INIT;
    }
    if (consumed_ || index_ >= valuesLen_) {
        return DBErr::E_ALREADY_CONSUMED;
    }
    void *values = nullptr;
    size_t valueLen = 0;
    auto status = OhCloudExtVectorGet(values_, index_, &values, reinterpret_cast<unsigned int *>(&valueLen));
    if (status != ERRNO_SUCCESS || values == nullptr) {
        return DBErr::E_ERROR;
    }
    OhCloudExtValueBucket *vBucket = reinterpret_cast<OhCloudExtValueBucket *>(values);
    auto data = GetData(vBucket);
    for (auto &[key, value] : data) {
        entry[key] = value;
    }
    entry[DBSchemaMeta::DELETE_FIELD] = GetExtend(vBucket, OPERATION_KEY);
    entry[DBSchemaMeta::GID_FIELD] = GetExtend(vBucket, GID_KEY);
    entry[DBSchemaMeta::CREATE_FIELD] = GetExtend(vBucket, CREATE_TIME_KEY);
    entry[DBSchemaMeta::MODIFY_FIELD] = GetExtend(vBucket, MODIFY_TIME_KEY);
    entry[DBSchemaMeta::CURSOR_FIELD] = cursor_;
    consumed_ = true;
    OhCloudExtValueBucketFree(vBucket);
    return DBErr::E_OK;
}

std::vector<std::pair<std::string, DBValue>> CloudCursorImpl::GetData(OhCloudExtValueBucket *vb)
{
    std::vector<std::pair<std::string, DBValue>> result {};
    OhCloudExtVector *keys = nullptr;
    size_t keysLen = 0;
    auto status = OhCloudExtValueBucketGetKeys(vb, &keys, reinterpret_cast<unsigned int *>(&keysLen));
    if (status != ERRNO_SUCCESS || keys == nullptr) {
        return result;
    }
    auto pKeys = std::shared_ptr<OhCloudExtVector>(keys, [](auto *keys) { OhCloudExtVectorFree(keys); });
    for (size_t i = 0; i < keysLen; i++) {
        void *value = nullptr;
        size_t valueLen = 0;
        auto status = OhCloudExtVectorGet(pKeys.get(), i, &value, reinterpret_cast<unsigned int *>(&valueLen));
        if (status != ERRNO_SUCCESS && value == nullptr) {
            return result;
        }
        char *str = reinterpret_cast<char *>(value);
        auto key = std::string(str, valueLen);
        if (EXTEND_KEYS.find(key) != EXTEND_KEYS.end()) {
            continue;
        }
        auto dBValue = ExtensionUtil::ConvertValues(vb, key);
        if (dBValue.index() == TYPE_INDEX<std::monostate>) {
            continue;
        }
        std::pair<std::string, DBValue> data { std::move(key),  std::move(dBValue) };
        result.push_back(std::move(data));
    }
    return result;
}

DBValue CloudCursorImpl::GetExtend(OhCloudExtValueBucket *vb, const std::string &col)
{
    DBValue result;
    int32_t status = ERRNO_SUCCESS;
    auto keyStr = reinterpret_cast<const unsigned char *>(col.c_str());
    OhCloudExtKeyName keyName = OhCloudExtKeyNameNew(keyStr, col.size());
    OhCloudExtValueType type = OhCloudExtValueType::VALUEINNERTYPE_EMPTY;
    void *content = nullptr;
    size_t ctLen = 0;
    status = OhCloudExtValueBucketGetValue(vb, keyName, &type, &content, reinterpret_cast<unsigned int *>(&ctLen));
    if (status != ERRNO_SUCCESS || content == nullptr) {
        return result;
    }
    if (col == OPERATION_KEY) {
        auto flag = *reinterpret_cast<int *>(content);
        result = (flag == DELETE) ? true : false;
    } else if (col == GID_KEY) {
        result = std::string(reinterpret_cast<char *>(content), ctLen);
    } else if (col == CREATE_TIME_KEY) {
        std::string createTime = std::string(reinterpret_cast<char *>(content), ctLen);
        int64_t create = strtoll(createTime.c_str(), nullptr, 10);
        result = create;
    } else if (col == MODIFY_TIME_KEY) {
        std::string modifyTime = std::string(reinterpret_cast<char *>(content), ctLen);
        int64_t modify = strtoll(modifyTime.c_str(), nullptr, 10);
        result = modify;
    }
    return result;
}

int32_t CloudCursorImpl::GetRow(DBVBucket &data)
{
    return GetEntry(data);
}

int32_t CloudCursorImpl::Get(int32_t col, DBValue &value)
{
    if (index_ == INVALID_INDEX) {
        return DBErr::E_NOT_INIT;
    }
    if (names_.size() <= static_cast<size_t>(col)) {
        return DBErr::E_INVALID_ARGS;
    }
    return Get(names_[col], value);
}

int32_t CloudCursorImpl::Get(const std::string &col, DBValue &value)
{
    if (col == DBSchemaMeta::CURSOR_FIELD) {
        value = cursor_;
        return DBErr::E_OK;
    }
    if (index_ == INVALID_INDEX) {
        return DBErr::E_NOT_INIT;
    }
    if (consumed_ || index_ >= valuesLen_) {
        return DBErr::E_ALREADY_CONSUMED;
    }

    void *data = nullptr;
    size_t valueLen = 0;
    auto status = OhCloudExtVectorGet(values_, index_, &data, reinterpret_cast<unsigned int *>(&valueLen));
    if (status != ERRNO_SUCCESS || data == nullptr) {
        return DBErr::E_ERROR;
    }
    OhCloudExtValueBucket *vBucket = reinterpret_cast<OhCloudExtValueBucket *>(data);
    auto it = std::find(names_.begin(), names_.end(), col);
    if (it == names_.end()) {
        value = GetExtend(vBucket, EXTEND_TO_KEYS[col]);
        return DBErr::E_OK;
    }
    auto pair = GetData(vBucket);
    OhCloudExtValueBucketFree(vBucket);
    for (auto &[first, second] : pair) {
        if (first == col) {
            value = second;
            return DBErr::E_OK;
        }
    }
    return DBErr::E_INVALID_ARGS;
}

int32_t CloudCursorImpl::Close()
{
    if (cloudData_ != nullptr) {
        OhCloudExtCloudDbDataFree(cloudData_);
        cloudData_ = nullptr;
    }
    if (values_ != nullptr) {
        OhCloudExtVectorFree(values_);
        values_ = nullptr;
    }
    index_ = INVALID_INDEX;
    return DBErr::E_OK;
}

bool CloudCursorImpl::IsEnd()
{
    return finished_;
}
}  // namespace OHOS::CloudData