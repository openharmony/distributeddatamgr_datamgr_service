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
#include <algorithm>
#include <climits>

#include "document_check.h"
#include "doc_errno.h"
#include "log_print.h"
#include "securec.h"

namespace DocumentDB {
namespace {
constexpr const char *KEY_ID = "_id";
constexpr const char *COLLECTION_PREFIX_GRD = "GRD_";
constexpr const char *COLLECTION_PREFIX_GM_SYS = "GM_SYS_";
const int MAX_COLLECTION_NAME = 511;
const int MAX_ID_LENS = 899;
const int JSON_DEEP_MAX = 4;

bool CheckCollectionNamePrefix(const std::string &name, const std::string &prefix)
{
    if (name.length() < prefix.length()) {
        return false;
    }

    auto itPrefix = prefix.begin();
    auto itName = name.begin();
    while (itPrefix != prefix.end()) {
        if (std::tolower(*itPrefix) != std::tolower(*itName)) {
            return false;
        }
        itPrefix++;
        itName++;
    }
    return true;
}
}

bool CheckCommon::CheckCollectionName(const std::string &collectionName, std::string &lowerCaseName, int &errCode)
{
    if (collectionName.empty()) {
        errCode = -E_INVALID_ARGS;
        return false;
    }
    if (collectionName.length() > MAX_COLLECTION_NAME) {
        errCode = -E_OVER_LIMIT;
        return false;
    }
    if (CheckCollectionNamePrefix(collectionName, COLLECTION_PREFIX_GRD) ||
        CheckCollectionNamePrefix(collectionName, COLLECTION_PREFIX_GM_SYS)) {
        GLOGE("Collection name is illegal");
        errCode = -E_INVALID_COLL_NAME_FORMAT;
        return false;
    }
    lowerCaseName = collectionName;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), [](unsigned char c){
        return std::tolower(c);
    });
    return true;
}

int CheckCommon::CheckFilter(JsonObject &filterObj)
{   
    if (filterObj.GetDeep() > JSON_DEEP_MAX) {
        GLOGE("filter's json deep is deeper than JSON_DEEP_MAX");
        return -E_INVALID_ARGS;
    }
    int ret = CheckIdFormat(filterObj);
    if (ret != E_OK) {
        GLOGE("Filter Id format is illegal");
        return ret;
    }
    if (!filterObj.GetChild().GetNext().IsNull()) {
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int CheckCommon::CheckIdFormat(JsonObject &filterJson)
{
    auto filterObjChild = filterJson.GetChild();
    auto idValue = JsonCommon::GetValueByFiled(filterObjChild, KEY_ID);
    if (idValue.GetValueType() != ValueObject::ValueType::VALUE_STRING) {
        return -E_INVALID_ARGS;
    }
    if (idValue.GetStringValue().length() > MAX_ID_LENS) {
        return -E_OVER_LIMIT;
    }
    return E_OK;
}

int CheckCommon::CheckDocument(JsonObject &documentObj)
{
    if (documentObj.GetDeep() > JSON_DEEP_MAX) {
        GLOGE("documentObj's json deep is deeper than JSON_DEEP_MAX");
        return -E_INVALID_ARGS;
    }
    int ret = CheckIdFormat(documentObj);
    if (ret != E_OK) {
        GLOGE("Document Id format is illegal");
        return ret;
    }
    if (!documentObj.GetChild().IsNull()) {
        auto documentObjChild = documentObj.GetChild();
        if (!JsonCommon::CheckJsonField(documentObjChild)) {
            GLOGE("Document json field format is illegal");
            return -E_INVALID_ARGS;
        }
    }
    return E_OK;
}

bool CheckCommon::CheckProjection(JsonObject &projectionObj, std::vector<std::vector<std::string>> &path)
{
    if (projectionObj.GetDeep() > JSON_DEEP_MAX) {
        GLOGE("projectionObj's json deep is deeper than JSON_DEEP_MAX");
        return -E_INVALID_ARGS;
    }
    if (!projectionObj.GetChild().IsNull()) {
        auto projectionObjChild = projectionObj.GetChild();
        if (!JsonCommon::CheckProjectionField(projectionObjChild)) {
            GLOGE("projection json field format is illegal");
            return false;
        }
    }
    for (int i = 0; i < path.size(); i++) {
        for (auto fieldName : path[i]) {
            for (int i = 0; i < fieldName.size(); i++) {
                if (!((isalpha(fieldName[i])) || (isdigit(fieldName[i])) || ('_' == fieldName[i]))) {
                    return false;
                }
                if (i == 0 && (isdigit(fieldName[i]))) {
                    return false;
                }
            }
        }
    }
    return true;
}
} // namespace DocumentDB