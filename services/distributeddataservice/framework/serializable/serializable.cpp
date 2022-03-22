/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
Serializable::json Serializable::Marshall() const
{
    json root;
    Marshal(root);
    return root;
}

bool Serializable::Unmarshall(const std::string &jsonStr)
{
    json jsonObj = json::parse(jsonStr, nullptr, false);
    if (jsonObj.is_discarded()) {
        // if the string size is less than 1, means the string is invalid.
        if (jsonStr.empty()) {
            return false;
        }
        jsonObj = json::parse(jsonStr.substr(1), nullptr, false); // drop first char to adapt A's value;
        if (jsonObj.is_discarded()) {
            return false;
        }
    }
    return Unmarshal(jsonObj);
}

Serializable::json Serializable::ToJson(const std::string &jsonStr)
{
    json jsonObj = json::parse(jsonStr, nullptr, false);
    if (jsonObj.is_discarded()) {
        // if the string size is less than 1, means the string is invalid.
        if (jsonStr.empty()) {
            return {};
        }
        jsonObj = json::parse(jsonStr.substr(1), nullptr, false); // drop first char to adapt A's value;
        if (jsonObj.is_discarded()) {
            return {};
        }
    }
    return jsonObj;
}

bool Serializable::GetValue(const json &node, const std::string &name, std::string &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_string()) {
        return false;
    }
    value = subNode;
    return true;
}

bool Serializable::GetValue(const json &node, const std::string &name, uint32_t &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_number_unsigned()) {
        return false;
    }
    subNode.get_to(value);
    return true;
}

bool Serializable::GetValue(const json &node, const std::string &name, int32_t &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_number_integer()) {
        return false;
    }
    subNode.get_to(value);
    return true;
}

bool Serializable::GetValue(const json &node, const std::string &name, int64_t &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_number_integer()) {
        return false;
    }
    subNode.get_to(value);
    return true;
}

bool Serializable::GetValue(const json &node, const std::string &name, bool &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_boolean()) {
        return false;
    }
    subNode.get_to(value);
    return true;
}

bool Serializable::GetValue(const json &node, const std::string &name, std::vector<uint8_t> &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_array()) {
        return false;
    }
    value = std::vector<uint8_t>(subNode);
    return true;
}

bool Serializable::GetValue(const json &node, const std::string &name, Serializable &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_object()) {
        return false;
    }
    return value.Unmarshal(subNode);
}

bool Serializable::SetValue(json &node, const std::string &value)
{
    node = value;
    return true;
}

bool Serializable::SetValue(json &node, const uint32_t &value)
{
    node = value;
    return true;
}

bool Serializable::SetValue(json &node, const int32_t &value)
{
    node = value;
    return true;
}

bool Serializable::SetValue(json &node, const int64_t &value)
{
    node = value;
    return true;
}

bool Serializable::SetValue(json &node, const bool &value)
{
    node = value;
    return true;
}

bool Serializable::SetValue(json &node, const std::vector<uint8_t> &value)
{
    node = value;
    return true;
}

bool Serializable::SetValue(json &node, const Serializable &value)
{
    return value.Marshal(node);
}

const Serializable::json &Serializable::GetSubNode(const json &node, const std::string &name)
{
    static const json jsonNull = json::value_t::null;
    if (node.is_discarded() || node.is_null()) {
        return jsonNull;
    }

    if (name.empty()) {
        return node;
    }

    auto it = node.find(name);
    if (it == node.end()) {
        return jsonNull;
    }
    return *it;
}
} // namespace DistributedData
} // namespace OHOS
