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

#ifndef DISTRIBUTEDDATAMGR_INOTIFY_MATRIX_FILE_H
#define DISTRIBUTEDDATAMGR_INOTIFY_MATRIX_FILE_H
#include "serializable/serializable.h"
#include "metadata/store_meta_data.h"

namespace OHOS::DistributedData {

struct API_EXPORT MatrixTableInfo final: public DistributedData::Serializable {
    uint64_t matrixOffset;
    uint64_t matrixValue;

    API_EXPORT MatrixTableInfo() = default;
    API_EXPORT MatrixTableInfo(uint64_t matrixOffset, uint64_t matrixValue)
        :matrixOffset(matrixOffset), matrixValue(matrixValue) {};
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};

struct API_EXPORT MatrixFileInfo final : public DistributedData::Serializable {
    int32_t version = 0;
    int32_t callingUid = 0;
    std::string rdbMetaKey;
    uint64_t fullSyncOffset = 0;
    uint64_t fullSyncValue = 0;
    // {tableName, tableInfo}
    std::map<std::string, MatrixTableInfo> matrixTables;
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
    API_EXPORT static std::string GenerateMatrixFileName(const StoreMetaData &meta);

    static constexpr const char *MATRIX_PREFIX = "matrix_";
    static constexpr const char *MATRIX_FILE_PATH = "/data/service/el1/public/for-all-app/matrix_files/";
};

} // namespace OHOS::DistributedData

#endif // DISTRIBUTEDDATAMGR_INOTIFY_MATRIX_FILE_H