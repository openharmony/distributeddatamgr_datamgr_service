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

MetaDataSaver::MetaDataSaver(bool async)
    : async_(async), flushed_(false)
{
}

MetaDataSaver::~MetaDataSaver()
{
    if (!flushed_ && !entries_.empty()) {
        Flush();
    }
}

bool MetaDataSaver::Flush()
{
    if (entries_.empty()) {
        flushed_ = true;
        return true;
    }

    auto success = MetaDataManager::GetInstance().SaveMeta(entries_, async_);
    if (!success) {
        ZLOGE("MetaDataSaver flush failed, count=%{public}zu, async=%{public}d",
              entries_.size(), async_);
    }
    flushed_ = true;
    return success;
}

} // namespace OHOS::DistributedData
