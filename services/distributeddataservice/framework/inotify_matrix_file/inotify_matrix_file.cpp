/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "inotify_matrix_file/inotify_matrix_file.h"

namespace OHOS::DistributedData {

bool MatrixTableInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(matrixOffset)], matrixOffset);
    SetValue(node[GET_NAME(matrixValue)], matrixValue);
    return true;
}

bool MatrixTableInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(matrixOffset), matrixOffset);
    GetValue(node, GET_NAME(matrixValue), matrixValue);
    return true;
}

bool MatrixFileInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(callingUid)], callingUid);
    SetValue(node[GET_NAME(rdbMetaKey)], rdbMetaKey);
    SetValue(node[GET_NAME(fullSyncOffset)], fullSyncOffset);
    SetValue(node[GET_NAME(fullSyncValue)], fullSyncValue);
    SetValue(node[GET_NAME(matrixTables)], matrixTables);
    return true;
}

bool MatrixFileInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(callingUid), callingUid);
    GetValue(node, GET_NAME(rdbMetaKey), rdbMetaKey);
    GetValue(node, GET_NAME(fullSyncOffset), fullSyncOffset);
    GetValue(node, GET_NAME(fullSyncValue), fullSyncValue);
    GetValue(node, GET_NAME(matrixTables), matrixTables);
    return true;
}

std::string MatrixFileInfo::GenerateMatrixFileName(const StoreMetaData &meta)
{
    std::string fileName = MATRIX_PREFIX;
    fileName.append(meta.user).append("_").append(meta.dataDir);
    for (char& c : fileName) {
        if (c == '/' || c == '.') {
            c = '_';
        }
    }
    return fileName;
}

} // namespace OHOS::DistributedData
