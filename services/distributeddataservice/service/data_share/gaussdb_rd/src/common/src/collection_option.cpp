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

#include "collection_option.h"

#include <algorithm>
#include <cstring>

#include "doc_errno.h"
#include "json_object.h"
#include "log_print.h"

namespace DocumentDB {
namespace {
constexpr const char *OPT_MAX_DOC = "maxdoc";

int CheckConfigValid(const JsonObject &config)
{
    JsonObject child = config.GetChild();
    while (!child.IsNull()) {
        std::string fieldName = child.GetItemField();
        if (strcmp(OPT_MAX_DOC, fieldName.c_str()) != 0) {
            GLOGE("Invalid collection config.");
            return -E_INVALID_CONFIG_VALUE;
        }
        child = child.GetNext();
    }
    return E_OK;
}
} // namespace

CollectionOption CollectionOption::ReadOption(const std::string &optStr, int &errCode)
{
    if (optStr.empty()) {
        return {};
    }

    std::string lowerCaseOptStr = optStr;
    std::transform(lowerCaseOptStr.begin(), lowerCaseOptStr.end(), lowerCaseOptStr.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    JsonObject collOpt = JsonObject::Parse(lowerCaseOptStr, errCode);
    if (errCode != E_OK) {
        GLOGE("Read collection option failed from str. %d", errCode);
        return {};
    }

    errCode = CheckConfigValid(collOpt);
    if (errCode != E_OK) {
        GLOGE("Check collection option, not support config item. %d", errCode);
        return {};
    }

    static const JsonFieldPath maxDocField = { OPT_MAX_DOC };
    if (!collOpt.IsFieldExists(maxDocField)) {
        return {};
    }

    ValueObject maxDocValue = collOpt.GetObjectByPath(maxDocField, errCode);
    if (errCode != E_OK) {
        GLOGE("Read collection option failed. %d", errCode);
        return {};
    }

    if (maxDocValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check collection option failed, the field type of maxDoc is not NUMBER.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    if (maxDocValue.GetIntValue() <= 0 || static_cast<uint64_t>(maxDocValue.GetIntValue()) > UINT32_MAX) {
        GLOGE("Check collection option failed, invalid maxDoc value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    CollectionOption option;
    option.maxDoc_ = static_cast<uint32_t>(maxDocValue.GetIntValue());
    option.option_ = optStr;
    return option;
}
} // namespace DocumentDB