/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_MATRIX_META_DATA_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_MATRIX_META_DATA_H
#include <string>
#include "serializable/serializable.h"
namespace OHOS::DistributedData {
class API_EXPORT MatrixMetaData final : public Serializable {
public:
    enum Origin : int32_t {
        LOCAL,
        REMOTE_RECEIVED,
        REMOTE_CONSISTENT,
    };

    static constexpr uint32_t DEFAULT_VERSION = 0;
    static constexpr uint32_t DEFAULT_LEVEL = 0x0;
    uint32_t version = DEFAULT_VERSION;
    uint16_t dynamic = DEFAULT_LEVEL;
    uint16_t statics = DEFAULT_LEVEL;
    int32_t origin = Origin::LOCAL;
    std::string deviceId;
    std::vector<std::string> dynamicInfo;
    std::vector<std::string> staticsInfo;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    std::string GetKey() const;
    std::string GetConsistentKey() const;
    API_EXPORT bool operator==(const MatrixMetaData &meta) const;
    API_EXPORT bool operator!=(const MatrixMetaData &meta) const;
    API_EXPORT static std::string GetPrefix(const std::initializer_list<std::string> &fields);

private:
    static constexpr const char *KEY_PREFIX = "MatrixMeta";
    static constexpr const char *RMOTE_CONSISTENT = "Consistent";
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_MATRIX_META_DATA_H
