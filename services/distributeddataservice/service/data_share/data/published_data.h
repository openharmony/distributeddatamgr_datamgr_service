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
class PublishedDataNode final : public VersionData {
public:
    PublishedDataNode();
    PublishedDataNode(const std::string &key, const std::string &bundleName, int64_t subscriberId,
        const std::variant<std::vector<uint8_t>, std::string> &value);
    ~PublishedDataNode() = default;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    std::string key;
    std::string bundleName;
    int64_t subscriberId;
    std::variant<std::vector<uint8_t>, std::string> value;
    std::time_t timestamp = 0;
};

class PublishedData final : public KvData {
public:
    // static std::vector<PublishedData> Query(const std::string &bundleName);
    static int32_t Query(const std::string &filter, std::variant<std::vector<uint8_t>, std::string> &publishedData);
    static std::string GenId(const std::string &key, const std::string &bundleName, int64_t subscriberId);
    PublishedData(const std::string &key, const std::string &bundleName, int64_t subscriberId,
        const std::variant<std::vector<uint8_t>, std::string> &value, const int version);
    ~PublishedData() = default;
    bool HasVersion() const override;
    int GetVersion() const override;
    std::string GetValue() const override;

private:
    PublishedDataNode value;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
