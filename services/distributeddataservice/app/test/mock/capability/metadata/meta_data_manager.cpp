/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "meta_data_manager.h"

namespace OHOS::DistributedData {
MetaDataManager &MetaDataManager::GetInstance()
{
    static MetaDataManager instance;
    return instance;
}

void MetaDataManager::Init(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = std::move(executors);
}

bool MetaDataManager::SaveMeta(const std::string &key, const CapMetaData &value)
{
    capabilities_.InsertOrAssign(key, value);
    executors_->Execute([this, key, value]() {
        if (capObserver_ != nullptr) {
            capObserver_(key, Serializable::Marshall(value), INSERT);
        }
    });
    return true;
}

bool MetaDataManager::LoadMeta(const std::string &key, CapMetaData &value)
{
    auto index = capabilities_.Find(key);
    if (index.first) {
        value = index.second;
        return true;
    }
    return false;
}

bool MetaDataManager::Subscribe(std::string prefix, Observer observer)
{
    (void)prefix;
    capObserver_ = observer;
    return true;
}

void MetaDataManager::UnSubscribe()
{
    capObserver_ = nullptr;
}

bool MetaDataManager::UpdateMeta(const std::string &key, const CapMetaData &value)
{
    capabilities_.InsertOrAssign(key, value);
    executors_->Execute([this, key, value]() {
        if (capObserver_ != nullptr) {
            capObserver_(key, Serializable::Marshall(value), UPDATE);
        }
    });
    return true;
}

bool MetaDataManager::DeleteMeta(const std::string &key, const CapMetaData &value)
{
    capabilities_.Erase(key);
    executors_->Execute([this, key, value]() {
        if (capObserver_ != nullptr) {
            capObserver_(key, Serializable::Marshall(value), DELETE);
        }
    });
    return true;
}
} // namespace OHOS::DistributedData