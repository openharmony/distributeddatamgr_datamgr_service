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

#include <memory>
#include <algorithm>

#include "doc_errno.h"
#include "json_object.h"
#include "log_print.h"

namespace DocumentDB {
namespace {
const std::string OPT_MAX_DOC = "maxdoc";
const std::vector<std::string> DB_CONFIG = {
    OPT_MAX_DOC,
};

bool CheckConfigSupport(const JsonObject &config, int &errCode)
{
    JsonObject child = config.GetChild();
    while (!child.IsNull()) {
        std::string fieldName = child.GetItemFiled();
        if (std::find(DB_CONFIG.begin(), DB_CONFIG.end(), fieldName) == DB_CONFIG.end()) {
            GLOGE("Invalid collection config.");
            errCode = -E_INVALID_CONFIG_VALUE;
            return false;
        }
        child = child.GetNext();
    }
    return true;
}
}
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

    if (!CheckConfigSupport(collOpt, errCode)) {
        GLOGE("Check collection option, not support config item. %d", errCode);
        return {};
    }

    static const JsonFieldPath maxDocField = {OPT_MAX_DOC};
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

uint32_t CollectionOption::GetMaxDoc() const
{
    return maxDoc_;
}

std::string CollectionOption::ToString() const
{
    return option_;
}

bool CollectionOption::operator==(const CollectionOption &targetOption) const
{
    return maxDoc_ == targetOption.maxDoc_;
}

bool CollectionOption::operator!=(const CollectionOption &targetOption) const
{
    return !(*this == targetOption);
}
}