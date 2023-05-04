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
#define LOG_TAG "KvAdaptor"
#include "kv_delegate.h"
#include "datashare_errno.h"
#include "directory_manager.h"
#include "ipc_skeleton.h"
#include "log_print.h"

namespace OHOS::DataShare {
int64_t KvDelegate::Upsert(const std::string &collectionName, const std::string &filter, const std::string &value)
{
    return E_OK;
}

int64_t KvDelegate::Delete(const std::string &collectionName, const std::string &filter)
{
    return E_OK;
}

bool KvDelegate::Init()
{
    if (isInitDone_) {
        return true;
    }
    isInitDone_ = true;
    return true;
}

KvDelegate::~KvDelegate()
{
}

int32_t KvDelegate::Upsert(const std::string &collectionName, const KvData &value)
{
    std::string id = DistributedData::Serializable::Marshall(*value.GetId());
    if (value.HasVersion() && value.GetVersion() != 0) {
        int version = -1;
        if (GetVersion(collectionName, id, version)) {
            if (value.GetVersion() <= version) {
                ZLOGE("GetVersion failed，%{public}s id %{private}s %{public}d %{public}d", collectionName.c_str(),
                      id.c_str(), static_cast<int>(value.GetVersion()), version);
                return E_VERSION_NOT_NEWER;
            }
        }
    }
    return Upsert(collectionName, id, DistributedData::Serializable::Marshall(value.GetValue()));
}

int32_t KvDelegate::DeleteById(const std::string &collectionName, const Id &id)
{
    return Delete(collectionName, DistributedData::Serializable::Marshall(id));
}

int32_t KvDelegate::Get(const std::string &collectionName, const Id &id, std::string &value)
{
    std::string filter = DistributedData::Serializable::Marshall(id);
    if (Get(collectionName, filter, "{}", value) != E_OK) {
        ZLOGE("Get failed, %{public}s %{public}s", collectionName.c_str(), filter.c_str());
        return false;
    }
    return true;
}

bool KvDelegate::GetVersion(const std::string &collectionName, const std::string &filter, int &version)
{
    std::string value;
    if (Get(collectionName, filter, "{}", value) != E_OK) {
        ZLOGE("Get failed, %{public}s %{public}s", collectionName.c_str(), filter.c_str());
        return false;
    }
    VersionData data(-1);
    if (!DistributedData::Serializable::Unmarshall(value, data)) {
        ZLOGE("Unmarshall failed，data %{public}s", value.c_str());
        return false;
    }
    version = data;
    return true;
}

int32_t KvDelegate::Get(
    const std::string &collectionName, const std::string &filter, const std::string &projection, std::string &result)
{
    return E_OK;
}

void KvDelegate::Flush()
{
}

int32_t KvDelegate::GetBatch(const std::string &collectionName, const std::string &filter,
    const std::string &projection, std::vector<std::string> &result)
{
    return E_OK;
}

KvDelegate::KvDelegate(const std::string &path) : path_(path) {}
} // namespace OHOS::DataShare