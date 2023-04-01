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

#include "doc_errno.h"
#include "json_object.h"
#include "log_print.h"

namespace DocumentDB {
CollectionOption CollectionOption::ReadOption(const std::string &optStr, int &errCode)
{
    if (optStr.empty()) {
        return {};
    }

    std::shared_ptr<JsonObject> collOpt;
    errCode = JsonObject::Parse(optStr, collOpt);
    if (errCode != E_OK) {
        GLOGE("Read collection option failed from str. %d", errCode);
        return {};
    }

    static const JsonFieldPath maxDocField = {"maxDoc"};
    if (!collOpt->IsFieldExists(maxDocField)) {
        return {};
    }

    ValueObject maxDocValue;
    errCode = collOpt->GetObjectByPath(maxDocField, maxDocValue);
    if (errCode != E_OK) {
        GLOGE("Read collection option failed. %d", errCode);
        return {};
    }

    if (maxDocValue.valueType != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check collection option failed, the field type of maxDoc is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    if (maxDocValue.intValue <= 0 || maxDocValue.intValue > UINT32_MAX) {
        GLOGE("Check collection option failed, invalid maxDoc value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    CollectionOption option;
    option.maxDoc_ = static_cast<uint32_t>(maxDocValue.intValue);
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