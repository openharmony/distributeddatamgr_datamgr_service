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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_SERIALIZABLE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_SERIALIZABLE_H
#include <string>
#include <vector>
#include "visibility.h"
#ifndef JSON_NOEXCEPTION
#define JSON_NOEXCEPTION
#endif
#include <variant>
#include <nlohmann/json.hpp>
namespace OHOS {
namespace DistributedData {
#ifndef GET_NAME
#define GET_NAME(value) #value
#endif
struct Serializable {
public:
    using json = nlohmann::json;
    using size_type = nlohmann::json::size_type;
    using error_handler_t = nlohmann::detail::error_handler_t;

    API_EXPORT json Marshall() const;
    template<typename T>
    static std::string Marshall(T &values)
    {
        json root;
        SetValue(root, values);
        return root.dump(-1, ' ', false, error_handler_t::replace);
    }

    API_EXPORT bool Unmarshall(const std::string &jsonStr);
    template<typename T>
    static bool Unmarshall(const std::string &body, T &values)
    {
        return GetValue(ToJson(body), "", values);
    }
    API_EXPORT static json ToJson(const std::string &jsonStr);
    API_EXPORT static bool IsJson(const std::string &jsonStr);
    virtual bool Marshal(json &node) const = 0;
    virtual bool Unmarshal(const json &node) = 0;
    API_EXPORT static bool GetValue(const json &node, const std::string &name, std::string &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, uint32_t &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, int32_t &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, int64_t &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, uint64_t &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, uint16_t &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, bool &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, std::vector<uint8_t> &value);
    API_EXPORT static bool GetValue(const json &node, const std::string &name, Serializable &value);
    API_EXPORT static bool SetValue(json &node, const std::string &value);
    API_EXPORT static bool SetValue(json &node, const uint32_t &value);
    API_EXPORT static bool SetValue(json &node, const int32_t &value);
    API_EXPORT static bool SetValue(json &node, const int64_t &value);
    API_EXPORT static bool SetValue(json &node, const double &value);
    API_EXPORT static bool SetValue(json &node, const uint64_t &value);
    API_EXPORT static bool SetValue(json &node, const uint16_t &value);
    // Use bool & to forbid the const T * auto convert to bool, const bool will convert to const uint32_t &value;
    template<typename T>
    API_EXPORT static std::enable_if_t<std::is_same_v<T, bool>, bool> SetValue(json &node, const T &value)
    {
        node = static_cast<bool>(value);
        return true;
    }

    API_EXPORT static bool SetValue(json &node, const std::vector<uint8_t> &value);
    API_EXPORT static bool SetValue(json &node, const Serializable &value);

    template<typename... _Types>
    API_EXPORT static bool SetValue(json &node, const std::variant<_Types...> &input);

    template<typename... _Types>
    API_EXPORT static bool GetValue(const json &node, const std::string &name, std::variant<_Types...> &value);
protected:
    API_EXPORT ~Serializable() = default;

    template<typename T>
    static bool GetValue(const json &node, const std::string &name, std::vector<T> &values);

    template<typename T>
    static bool SetValue(json &node, const std::vector<T> &values);

    template<typename T>
    static bool GetValue(const json &node, const std::string &name, std::map<std::string, T> &values);

    template<typename T>
    static bool SetValue(json &node, const std::map<std::string, T> &values);

    template<typename T>
    static bool GetValue(const json &node, const std::string &name, T *&value);

    template<typename T>
    static bool SetValue(json &node, const T *value);

    template<typename _OutTp>
    static bool ReadVariant(const json &node, const std::string &name, uint32_t step, uint32_t index, _OutTp &output);

    template<typename _OutTp, typename _First, typename... _Rest>
    static bool ReadVariant(const json &node, const std::string &name, uint32_t step, uint32_t index, _OutTp &output);

    template<typename _InTp>
    static bool WriteVariant(json &node, uint32_t step, const _InTp &input);

    template<typename _InTp, typename _First, typename... _Rest>
    static bool WriteVariant(json &node, uint32_t step, const _InTp &input);
    API_EXPORT static const json &GetSubNode(const json &node, const std::string &name);
};

template<typename T>
bool Serializable::GetValue(const json &node, const std::string &name, std::vector<T> &values)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_array()) {
        return false;
    }
    bool result = true;
    values.resize(subNode.size());
    for (size_type i = 0; i < subNode.size(); ++i) {
        result = GetValue(subNode[i], "", values[i]) && result;
    }
    return result;
}

template<typename T>
bool Serializable::SetValue(json &node, const std::vector<T> &values)
{
    bool result = true;
    size_type i = 0;
    node = json::value_t::array;
    for (const auto &value : values) {
        result = SetValue(node[i], value) && result;
        i++;
    }
    return result;
}

template<typename T>
bool Serializable::GetValue(const json &node, const std::string &name, std::map<std::string, T> &values)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null() || !subNode.is_object()) {
        return false;
    }
    bool result = true;
    for (auto object = subNode.begin(); object != subNode.end(); ++object) {
        result = GetValue(object.value(), "", values[object.key()]) && result;
    }
    return result;
}

template<typename T>
bool Serializable::SetValue(json &node, const std::map<std::string, T> &values)
{
    bool result = true;
    node = json::value_t::object;
    for (const auto &[key, value] : values) {
        result = SetValue(node[key], value) && result;
    }
    return result;
}

template<typename T>
bool Serializable::GetValue(const json &node, const std::string &name, T *&value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null()) {
        return false;
    }
    value = new(std::nothrow) T();
    if (value == nullptr) {
        return false;
    }
    bool result = GetValue(subNode, "", *value);
    if (!result) {
        delete value;
        value = nullptr;
    }
    return result;
}

template<typename T>
bool Serializable::SetValue(json &node, const T *value)
{
    if (value == nullptr) {
        return false;
    }
    return SetValue(node, *value);
}

template<typename... _Types>
bool Serializable::SetValue(json &node, const std::variant<_Types...> &input)
{
    bool ret = SetValue(node[GET_NAME(type)], input.index());
    if (!ret) {
        return ret;
    }
    return WriteVariant<decltype(input), _Types...>(node[GET_NAME(value)], 0, input);
}

template<typename... _Types>
bool Serializable::GetValue(const json &node, const std::string &name, std::variant<_Types...> &value)
{
    auto &subNode = GetSubNode(node, name);
    if (subNode.is_null()) {
        return false;
    }
    uint32_t index;
    bool ret = GetValue(subNode, GET_NAME(type), index);
    if (!ret) {
        return ret;
    }

    return Serializable::ReadVariant<decltype(value), _Types...>(subNode, GET_NAME(value), 0, index, value);
}

template<typename _InTp>
bool Serializable::WriteVariant(json &node, uint32_t step, const _InTp &input)
{
    return false;
}

template<typename _OutTp, typename _First, typename... _Rest>
bool Serializable::ReadVariant(const json &node, const std::string &name, uint32_t step, uint32_t index, _OutTp &output)
{
    if (step == index) {
        _First result;
        if (!Serializable::GetValue(node, name, result)) {
            return false;
        }
        output = result;
        return true;
    }
    return Serializable::ReadVariant<_OutTp, _Rest...>(node, name, step + 1, index, output);
}

template<typename _InTp, typename _First, typename... _Rest>
bool Serializable::WriteVariant(json &node, uint32_t step, const _InTp &input)
{
    if (step == input.index()) {
        return Serializable::SetValue(node, std::get<_First>(input));
    }
    return WriteVariant<_InTp, _Rest...>(node, step + 1, input);
}

template<typename _OutTp>
bool Serializable::ReadVariant(const json &node, const std::string &name, uint32_t step, uint32_t index, _OutTp &output)
{
    return false;
}
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_SERIALIZABLE_H
