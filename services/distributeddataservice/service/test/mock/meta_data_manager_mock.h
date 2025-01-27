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
#ifndef OHOS_META_DATA_MANAGER_MOCK_H
#define OHOS_META_DATA_MANAGER_MOCK_H

#include <gmock/gmock.h>
#include "metadata/meta_data_manager.h"

namespace OHOS::DistributedData {
class BMetaDataManager {
public:
    virtual bool LoadMeta(const std::string &, Serializable &, bool) = 0;
    BMetaDataManager() = default;
    virtual ~BMetaDataManager() = default;
private:
    static inline std::shared_ptr<BMetaDataManager> metaDataManager = nullptr;
};

class MetaDataManagerMock : public BMetaDataManager {
public:
    MOCK_METHOD(bool, LoadMeta, (const std::string &, Serializable &, bool));
};
}
#endif //OHOS_META_DATA_MANAGER_MOCK_H
