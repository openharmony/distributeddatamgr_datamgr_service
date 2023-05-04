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

#ifndef DATASHARESERVICE_TEMPLATE_DATA_H
#define DATASHARESERVICE_TEMPLATE_DATA_H

#include "datashare_template.h"
#include "db_delegate.h"
#include "serializable/serializable.h"

namespace OHOS::DataShare {
struct TemplateNode : public DistributedData::Serializable {
    TemplateNode() = default;
    explicit TemplateNode(const Template &tpl);
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    Template tpl;
};

struct TemplateRootNode : public DistributedData::Serializable {
    TemplateRootNode() = default;
    TemplateRootNode(const std::string &uri, const std::string &bundleName, int64_t subscriberId, const Template &tpl);
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    std::string uri;
    std::string bundleName;
    int64_t subscriberId;
    TemplateNode tpl;
};

struct TemplateData final : public KvData {
    TemplateData() = default;
    TemplateData(const std::string &uri, const std::string &bundleName, int64_t subscriberId, const Template &tpl);
    std::shared_ptr<Id> GetId() const override;
    bool HasVersion() const override;
    VersionData GetVersion() const override;
    const DistributedData::Serializable &GetValue() const override;
    bool Unmarshal(const json &node) override;
    TemplateRootNode value;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
