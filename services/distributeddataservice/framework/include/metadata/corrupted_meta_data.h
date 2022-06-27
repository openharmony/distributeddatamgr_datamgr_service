/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_CORRUPTED_META_DATA_H
#define DISTRIBUTEDDATAMGR_CORRUPTED_META_DATA_H
#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
struct API_EXPORT CorruptedMetaData final : public Serializable {
    std::string appId = "";
    std::string bundleName = "";
    std::string storeId = "";
    bool isCorrupted = false;
    API_EXPORT CorruptedMetaData();
    API_EXPORT CorruptedMetaData(const std::string &appId, const std::string &bundleName, const std::string &storeId);
    API_EXPORT ~CorruptedMetaData();
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;

    API_EXPORT std::string GetKey();
private:
    static constexpr const char *KEY_PREFIX = "CorruptedMetaData";
};
} // namespace DistributedData
} // namespace OHOS

#endif // DISTRIBUTEDDATAMGR_CORRUPTED_META_DATA_H
