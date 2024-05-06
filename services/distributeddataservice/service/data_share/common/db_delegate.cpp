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

#define LOG_TAG "DBAdaptor"
#include "db_delegate.h"

#include "kv_delegate.h"
#include "log_print.h"
#include "rdb_delegate.h"
namespace OHOS::DataShare {
std::shared_ptr<DBDelegate> DBDelegate::Create(const std::string &dir, int version, bool registerFunction,
    bool isEncrypt, const std::string &secretMetaKey)
{
    return std::make_shared<RdbDelegate>(dir, version, registerFunction, isEncrypt, secretMetaKey);
}

std::shared_ptr<DBDelegate> DBDelegate::Create(DistributedData::StoreMetaData &metaData)
{
    return std::make_shared<RdbDelegate>(metaData.dataDir, NO_CHANGE_VERSION, true,
        metaData.isEncrypt, metaData.isEncrypt ? metaData.GetSecretKey() : "");
}

std::shared_ptr<KvDBDelegate> KvDBDelegate::GetInstance(
    bool reInit, const std::string &dir, const std::shared_ptr<ExecutorPool> &executors)
{
    static std::shared_ptr<KvDBDelegate> delegate = nullptr;
    static std::mutex mutex;
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (delegate == nullptr || reInit) {
        delegate = std::make_shared<KvDelegate>(dir, executors);
    }
    return delegate;
}

bool Id::Marshal(DistributedData::Serializable::json &node) const
{
    auto ret = false;
    if (userId == INVALID_USER) {
        ret = SetValue(node[GET_NAME(_id)], _id);
    } else {
        ret = SetValue(node[GET_NAME(_id)], _id + "_" + std::to_string(userId));
    }
    return ret;
}

bool Id::Unmarshal(const DistributedData::Serializable::json &node)
{
    return false;
}

Id::Id(const std::string &id, const int32_t userId) : _id(id), userId(userId) {}

VersionData::VersionData(int version) : version(version) {}

bool VersionData::Unmarshal(const DistributedData::Serializable::json &node)
{
    return GetValue(node, GET_NAME(version), version);
}

bool VersionData::Marshal(DistributedData::Serializable::json &node) const
{
    return SetValue(node[GET_NAME(version)], version);
}

const std::string &KvData::GetId() const
{
    return id;
}

KvData::KvData(const Id &id) : id(DistributedData::Serializable::Marshall(id)) {}
} // namespace OHOS::DataShare