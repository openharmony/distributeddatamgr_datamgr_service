/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "metadata/meta_data_saver.h"
#include "log_print.h"

namespace OHOS::DistributedData {

MetaDataSaver::MetaDataSaver(bool isLocal)
    : isLocal_(isLocal)
{
}

MetaDataSaver::~MetaDataSaver()
{
    if (!entries_.empty()) {
        auto success = MetaDataManager::GetInstance().SaveMeta(entries_, isLocal_);
        if (!success) {
            ZLOGE("MetaDataSaver auto-flush failed, count=%{public}zu, isLocal=%{public}d",
                  entries_.size(), isLocal_);
        }
        entries_.clear();  // Clear entries regardless of success/failure
    }
}

size_t MetaDataSaver::Size() const
{
    return entries_.size();
}

void MetaDataSaver::Clear()
{
    entries_.clear();
}

void MetaDataSaver::Add(const std::string &key, const std::string &value)
{
    entries_.push_back({key, value});
}

} // namespace OHOS::DistributedData
