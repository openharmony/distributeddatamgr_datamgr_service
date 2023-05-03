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
#define LOG_TAG "TemplateData"
#include "template_data.h"

namespace OHOS::DataShare {
bool TemplateNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret;
    for (auto &predicate : tpl.predicates_) {
        ret = SetValue(node["predicates"][predicate.key_], predicate.selectSql_);
    }
    ret = ret && SetValue(node["scheduler"], tpl.scheduler_);
    return ret;
}

bool TemplateNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    auto &predicatesNode = node["predicates"];
    for (auto it = predicatesNode.begin(); it != predicatesNode.end(); ++it) {
        tpl.predicates_.emplace_back(static_cast<std::string>(it.key()), static_cast<std::string>(it.value()));
    }
    bool ret = GetValue(node, "scheduler", tpl.scheduler_);
    return ret;
}

TemplateNode::TemplateNode(const Template &tpl) : tpl(tpl) {}

bool TemplateRootNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node["uri"], uri);
    ret = ret && SetValue(node["bundleName"], bundleName);
    ret = ret && SetValue(node["subscriberId"], subscriberId);
    ret = ret && SetValue(node["template"], tpl);
    return ret;
}

bool TemplateRootNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, "uri", uri);
    ret = ret && GetValue(node, "bundleName", bundleName);
    ret = ret && GetValue(node, "subscriberId", subscriberId);
    ret = ret && GetValue(node, "template", tpl);
    return ret;
}

TemplateRootNode::TemplateRootNode(
    const std::string &uri, const std::string &bundleName, int64_t subscriberId, const Template &tpl)
    : uri(uri), bundleName(bundleName), subscriberId(subscriberId), tpl(tpl)
{
}

std::shared_ptr<Id> TemplateData::GetId() const
{
    return std::make_shared<Id>(value.uri + "_" + std::to_string(value.subscriberId) + "_" + value.bundleName);
}

bool TemplateData::HasVersion() const
{
    return false;
}

VersionData TemplateData::GetVersion() const
{
    return VersionData(0);
}

const DistributedData::Serializable &TemplateData::GetValue() const
{
    return value;
}

bool TemplateData::Unmarshal(const DistributedData::Serializable::json &node)
{
    return value.Unmarshal(node);
}

TemplateData::TemplateData(
    const std::string &uri, const std::string &bundleName, int64_t subscriberId, const Template &tpl)
    : value(uri, bundleName, subscriberId, tpl)
{
}
} // namespace OHOS::DataShare