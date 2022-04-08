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

#define LOG_TAG "KvStoreSnapshotClient"

#include "kvstore_snapshot_client.h"
#include "constant.h"
#include "dds_trace.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
KvStoreSnapshotClient::KvStoreSnapshotClient() : kvStoreSnapshotProxy_(nullptr)
{}

KvStoreSnapshotClient::KvStoreSnapshotClient(sptr<IKvStoreSnapshotImpl> kvStoreSnapshotProxy)
    : kvStoreSnapshotProxy_(std::move(kvStoreSnapshotProxy))
{
    ZLOGI("construct");
}

KvStoreSnapshotClient::~KvStoreSnapshotClient()
{
    ZLOGI("destruct");
}

Status KvStoreSnapshotClient::GetEntries(const Key &prefixKey, Key &nextKey, std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);

    std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(prefixKey.Data());
    if (keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid prefixKey.");
        return Status::INVALID_ARGUMENT;
    }
    keyData = Constant::TrimCopy<std::vector<uint8_t>>(nextKey.Data());
    if (keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid nextKey.");
        return Status::INVALID_ARGUMENT;
    }
    Status status = Status::SERVER_UNAVAILABLE;
    if (kvStoreSnapshotProxy_ != nullptr) {
        kvStoreSnapshotProxy_->GetEntries(prefixKey, nextKey,
            [&status, &entries, &nextKey](Status stat, auto &result, const auto &next) {
                status = stat;
                entries = std::move(result);
                nextKey = next;
            });
    } else {
        ZLOGE("snapshot proxy is nullptr.");
    }
    return status;
}

Status KvStoreSnapshotClient::GetEntries(const Key &prefixKey, std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);

    std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(prefixKey.Data());
    if (keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid prefixKey.");
        return Status::INVALID_ARGUMENT;
    }
    if (kvStoreSnapshotProxy_ == nullptr) {
        ZLOGE("snapshot proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    Key startKey("");
    Status status = Status::ERROR;
    do {
        kvStoreSnapshotProxy_->GetEntries(prefixKey, startKey,
            [&status, &entries, &startKey](Status stat, auto &result, Key next) {
                status = stat;
                if (stat != Status::SUCCESS) {
                    return;
                }
                startKey = result.empty() ? Key("") : next;
                entries.insert(entries.end(), result.begin(), result.end());
            });
    } while (status == Status::SUCCESS && startKey.ToString() != "");
    if (status != Status::SUCCESS) {
        ZLOGW("Error occurs during GetEntries.");
        entries.clear();
    }
    return status;
}

Status KvStoreSnapshotClient::GetKeys(const Key &prefixKey, Key &nextKey, std::vector<Key> &keys)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(prefixKey.Data());
    if (keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid prefixKey.");
        return Status::INVALID_ARGUMENT;
    }
    keyData = Constant::TrimCopy<std::vector<uint8_t>>(nextKey.Data());
    if (keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid nextKey.");
        return Status::INVALID_ARGUMENT;
    }
    Status status = Status::SERVER_UNAVAILABLE;
    if (kvStoreSnapshotProxy_ != nullptr) {
        kvStoreSnapshotProxy_->GetKeys(prefixKey, nextKey,
            [&status, &keys, &nextKey](Status stat, auto &result, const auto &next) {
                status = stat;
                keys = std::move(result);
                nextKey = next;
            });
    } else {
        ZLOGE("snapshot proxy is nullptr.");
    }
    return status;
}

Status KvStoreSnapshotClient::GetKeys(const Key &prefixKey, std::vector<Key> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(prefixKey.Data());
    if (keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid prefixKey.");
        return Status::INVALID_ARGUMENT;
    }
    if (kvStoreSnapshotProxy_ == nullptr) {
        ZLOGE("snapshot proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    Key startKey("");
    Status status = Status::ERROR;
    do {
        kvStoreSnapshotProxy_->GetKeys(prefixKey, startKey,
            [&status, &entries, &startKey](Status stat, auto &result, Key next) {
                status = stat;
                if (stat != Status::SUCCESS) {
                    return;
                }
                startKey = result.empty() ? Key("") : next;
                entries.insert(entries.end(), result.begin(), result.end());
            });
    } while (status == Status::SUCCESS && startKey.ToString() != "");
    if (status != Status::SUCCESS) {
        ZLOGW("Error occurs during GetKeys.");
        entries.clear();
    }
    return status;
}

Status KvStoreSnapshotClient::Get(const Key &key, Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);

    std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(key.Data());
    if (keyData.size() == 0 || keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid key.");
        return Status::INVALID_ARGUMENT;
    }
    if (kvStoreSnapshotProxy_ != nullptr) {
        return kvStoreSnapshotProxy_->Get(key, value);
    }
    ZLOGE("snapshot proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

sptr<IKvStoreSnapshotImpl> KvStoreSnapshotClient::GetkvStoreSnapshotProxy()
{
    return kvStoreSnapshotProxy_;
}
}  // namespace DistributedKv
}  // namespace OHOS
