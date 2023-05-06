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
#include "rdb_delegate.h"
namespace OHOS::DataShare {
std::shared_ptr<DBDelegate> DBDelegate::Create(const std::string &dir, int version, int &errCode, bool registerFunction)
{
    return std::make_shared<RdbDelegate>(dir, version, errCode, registerFunction);
}

const std::string KvDBDelegate::TEMPLATE_TABLE = "template_";
const std::string KvDBDelegate::DATA_TABLE = "data_";

std::shared_ptr<KvDBDelegate> KvDBDelegate::GetInstance(bool reInit, const std::string &dir)
{
    static std::shared_ptr<KvDBDelegate> delegate = nullptr;
    static std::mutex mutex;
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (delegate == nullptr || reInit) {
        delegate = std::make_shared<KvDelegate>(dir);
    }
    return delegate;
}

bool KvData::Marshal(DistributedData::Serializable::json &node) const
{
    auto ret = SetValue(node, *GetId());
    if (HasVersion()) {
        ret &= SetValue(node, GetVersion());
    }
    return ret & SetValue(node, GetValue());
}

bool Id::Marshal(DistributedData::Serializable::json &node) const
{
    return SetValue(node[GET_NAME(_id)], _id);
}

bool Id::Unmarshal(const DistributedData::Serializable::json &node)
{
    return GetValue(node, GET_NAME(_id), _id);
}

Id::Id(const std::string &id) : _id(id) {}

VersionData::VersionData(int version) : version(version) {}

bool VersionData::Unmarshal(const DistributedData::Serializable::json &node)
{
    return GetValue(node, "version", version);
}
bool VersionData::Marshal(DistributedData::Serializable::json &node) const
{
    return SetValue(node["version"], version);
}
} // namespace OHOS::DataShare