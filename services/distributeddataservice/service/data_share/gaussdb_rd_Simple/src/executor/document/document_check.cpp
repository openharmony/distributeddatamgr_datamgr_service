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

#include "doc_errno.h"
#include "log_print.h"
#include "securec.h"
#include "document_check.h"

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
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), [](unsigned char c) {
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
    if (!filterObj.GetChild().IsNull()) {
        auto filterObjChild = filterObj.GetChild();
        if (!JsonCommon::CheckJsonField(filterObjChild)) {
            GLOGE("filter json field format is illegal");
            return -E_INVALID_ARGS;
        }
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

int CheckCommon::CheckFilter(JsonObject &filterObj, bool &isOnlyId, std::vector<std::vector<std::string>> &filterPath)
{
    for (int i = 0; i < filterPath.size(); i++) {
        if (filterPath[i].size() > JSON_DEEP_MAX) {
            GLOGE("filter's json deep is deeper than JSON_DEEP_MAX");
            return -E_INVALID_ARGS;
        }
    }
    if (!filterObj.GetChild().GetNext().IsNull()) {
        isOnlyId = false;
    }
    for (int i = 0; i < filterPath.size(); i++) {
        for (auto fieldName : filterPath[i]) {
            for (int j = 0; j < fieldName.size(); j++) {
                if (!((isalpha(fieldName[j])) || (isdigit(fieldName[j])) || ('_' == fieldName[j]))) {
                    return -E_INVALID_ARGS;
                }
            }
        }
    }
    bool isIdExisit = false;
    int ret = CheckIdFormat(filterObj, isIdExisit);
    if (ret != E_OK) {
        GLOGE("Filter Id format is illegal");
        return ret;
    }
    if (!isIdExisit) {
        isOnlyId = false;
    }
    return E_OK;
}

bool CheckCommon::CheckFilter(const std::string &filter, std::string &idStr, int &errCode)
{
    if (filter.empty()) {
        errCode = -E_INVALID_ARGS;
        GLOGE("Check filter invalid. %d", errCode);
        return false;
    }

    JsonObject filterObject = JsonObject::Parse(filter, errCode, true);
    if (errCode != E_OK) {
        GLOGE("Parse filter failed. %d", errCode);
        return false;
    }

    JsonObject filterId = filterObject.GetObjectItem("_id", errCode);
    if (errCode != E_OK || filterId.GetItemValue().GetValueType() != ValueObject::ValueType::VALUE_STRING) {
        GLOGE("Check filter '_id' not found or type not string.");
        errCode = -E_INVALID_ARGS;
        return false;
    }

    idStr = filterId.GetItemValue().GetStringValue();
    return true;
}

bool CheckCommon::CheckDocument(const std::string &updateStr, int &errCode)
{
    if (updateStr.empty()) {
        errCode = -E_INVALID_ARGS;
        return false;
    }

    JsonObject updateObj = JsonObject::Parse(updateStr, errCode);
    if (updateObj.IsNull() || errCode != E_OK) {
        GLOGE("Parse update document failed. %d", errCode);
        return false;
    }
    std::vector<std::vector<std::string>> updatePath;
    updatePath = JsonCommon::ParsePath(updateObj, errCode);
    if (errCode != E_OK) {
        return false;
    }
    for (auto singlePath : updatePath) {
        if (singlePath.size() > JSON_DEEP_MAX) {
            GLOGE("filter's json deep is deeper than JSON_DEEP_MAX");
            errCode = -E_INVALID_ARGS;
            return false;
        }
    }

    JsonObject filterId = updateObj.GetObjectItem("_id", errCode);
    if (errCode != -E_NOT_FOUND) {
        GLOGE("Can not change '_id' with update document failed.");
        errCode = -E_INVALID_ARGS;
        return false;
    }

    return true;
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

int CheckCommon::CheckIdFormat(JsonObject &filterJson, bool &isIdExisit)
{
    auto filterObjChild = filterJson.GetChild();
    ValueObject idValue = JsonCommon::GetValueByFiled(filterObjChild, KEY_ID, isIdExisit);
    if ((idValue.GetValueType() == ValueObject::ValueType::VALUE_NULL) && isIdExisit == false) {
        return E_OK;
    }
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
        GLOGE("documentObj is =======>%s", documentObj.Print().c_str());
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

bool CheckCommon::CheckUpdata(JsonObject &updataObj, std::vector<std::vector<std::string>> &path)
{
    if (updataObj.GetDeep() > JSON_DEEP_MAX) {
        GLOGE("projectionObj's json deep is deeper than JSON_DEEP_MAX");
        return -E_INVALID_ARGS;
    }
    if (!updataObj.GetChild().IsNull()) {
        auto updataObjChild = updataObj.GetChild();
        if (!JsonCommon::CheckProjectionField(updataObjChild)) {
            GLOGE("projection json field format is illegal");
            return false;
        }
    }
    for (int i = 0; i < path.size(); i++) {
        for (auto fieldName : path[i]) {
            for (int j = 0; j < fieldName.size(); j++) {
                if (!((isalpha(fieldName[j])) || (isdigit(fieldName[j])) || ('_' == fieldName[j]))) {
                    return false;
                }
            }
        }
    }
    for (auto singlePath: path) {
        if (singlePath.size() > 4) {
            return false;
        }
    }
    bool isIdExist = true;
    CheckIdFormat(updataObj, isIdExist);
    if (isIdExist) {
        return false;
    }
    return true;
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
            for (int j = 0; j < fieldName.size(); j++) {
                if (!((isalpha(fieldName[j])) || (isdigit(fieldName[j])) || ('_' == fieldName[j]))) {
                    return false;
                }
                if (j == 0 && (isdigit(fieldName[j]))) {
                    return false;
                }
            }
        }
    }
    return true;
}
} // namespace DocumentDB