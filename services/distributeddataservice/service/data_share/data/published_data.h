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

#ifndef DATASHARESERVICE_PUBLISHED_DATA_H
#define DATASHARESERVICE_PUBLISHED_DATA_H

#include "db_delegate.h"
#include "serializable/serializable.h"

namespace OHOS::DataShare {
struct PublishedDataNode final : public DistributedData::Serializable {
    PublishedDataNode(const std::string &key, const std::string &bundleName, int64_t subscriberId,
        const std::variant<sptr<Ashmem>, std::string> &value, const int version);
    ~PublishedDataNode() = default;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    std::string key;
    std::string bundleName;
    int64_t subscriberId;
    std::variant<sptr<Ashmem>, std::string> value;
    VersionData version;
};

class PublishedData final : public KvData {
public:
    static std::vector<PublishedData> Query(const std::string &bundleName);
    static int32_t Query(const std::string &filter, std::variant<sptr<Ashmem>, std::string> &publishedData);
    explicit PublishedData(const std::string &key = "", const std::string &bundleName = "", int64_t subscriberId = 0,
        const std::variant<sptr<Ashmem>, std::string> &value = "", const int version = 0);
    ~PublishedData() = default;
    std::shared_ptr<Id> GetId() const override
    {
        return std::make_shared<Id>(value.key + "_" + std::to_string(value.subscriberId) + "_" + value.bundleName);
    }
    bool HasVersion() const override;
    VersionData GetVersion() const override;
    const DistributedData::Serializable &GetValue() const override;
    bool Unmarshal(const json &node) override;
    static constexpr int8_t STRING = 0;
    static constexpr int8_t ASHMEM = 1;
    PublishedDataNode value;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
