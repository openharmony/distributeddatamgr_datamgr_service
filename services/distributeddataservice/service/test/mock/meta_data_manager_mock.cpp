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
#include "meta_data_manager_mock.h"

namespace OHOS::DistributedData {

OHOS::DistributedData::MetaDataManager &OHOS::DistributedData::MetaDataManager::GetInstance()
{
    static MetaDataManager instance;
    return instance;
}

OHOS::DistributedData::MetaDataManager::MetaDataManager() { }

OHOS::DistributedData::MetaDataManager::~MetaDataManager() { }

bool OHOS::DistributedData::MetaDataManager::LoadMeta(const std::string &key, Serializable &value, bool isLocal)
{
    return BMetaDataManager::metaDataManager->LoadMeta(key, value, isLocal);
}
}