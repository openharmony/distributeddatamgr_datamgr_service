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
struct PredicatesNode final: public DistributedData::Serializable {
    PredicatesNode() = default;
    PredicatesNode(const std::string &key, const std::string &selectSql);
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    std::string key;
    std::string selectSql;
};
struct TemplateNode final: public DistributedData::Serializable {
    TemplateNode() = default;
    explicit TemplateNode(const Template &tpl);
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    Template ToTemplate() const;
private:
    std::vector<PredicatesNode> predicates;
    std::string scheduler;
};

struct TemplateRootNode final: public DistributedData::Serializable {
    TemplateRootNode() = default;
    TemplateRootNode(const std::string &uri, const std::string &bundleName, const int64_t subscriberId,
        const int32_t userId, const Template &tpl);
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    Template ToTemplate() const;
private:
    std::string uri;
    std::string bundleName;
    int64_t subscriberId;
    int32_t userId;
    TemplateNode tpl;
};

struct TemplateData final : public KvData {
    TemplateData(const std::string &uri, const std::string &bundleName, int64_t subscriberId, int32_t userId,
        const Template &tpl);
    static int32_t Query(const std::string &filter, Template &aTemplate);
    static std::string GenId(const std::string &uri, const std::string &bundleName, int64_t subscriberId);
    static bool Delete(const std::string &bundleName, const int32_t userId);
    static bool Add(const std::string &uri, const int32_t userId, const std::string &bundleName,
        const int64_t subscriberId, const Template &aTemplate);
    static bool Delete(
        const std::string &uri, const int32_t userId, const std::string &bundleName, const int64_t subscriberId);
    bool HasVersion() const override;
    int GetVersion() const override;
    std::string GetValue() const override;

private:
    TemplateRootNode value;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
