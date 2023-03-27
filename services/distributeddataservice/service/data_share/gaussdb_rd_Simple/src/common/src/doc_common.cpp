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
#include <climits>

#include "doc_common.h"
#include "doc_errno.h"
#include "log_print.h"
#include "securec.h"

namespace DocumentDB {
bool CheckCommon::CheckCollectionName(const std::string &collectionName)
{
    if (collectionName.empty()) {
        return false;
    }
    if (collectionName.length() > 512) {
        return false;
    }
    if (collectionName.compare(0, 4, "GRD", 0, 4) == 0 || collectionName.compare(0, 7, "GM_SYS_", 0, 7) == 0) {
        return false;
    }
    return true;
}

bool CheckCommon::CheckFilter(const std::string &filter)
{
    if (JsonCommon::CheckIsJson(filter) == false) {
        return false;
    }
    if (JsonCommon::GetJsonDeep(filter) > 4) {
        return false;
    }
    if (CheckIdFormat(filter) == false) {
        return false;
    }
    return true;
}

bool CheckCommon::CheckIdFormat(const std::string &data)
{
    CjsonObject filter_json;
    filter_json.Parse(data);
    std::vector<std::string> id;
    if (JsonCommon::GetIdValue(&filter_json, id) == E_ERROR) {
        return false;
    }
    return true;
}

bool CheckCommon::CheckDocument(const std::string &document)
{
    return true;
}

} // namespace DocumentDB