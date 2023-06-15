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

#ifndef DATASHARESERVICE_RESULTSET_JSON_FORMATTER_H
#define DATASHARESERVICE_RESULTSET_JSON_FORMATTER_H

#include "datashare_template.h"
#include "rdb_utils.h"
#include "serializable/serializable.h"

namespace OHOS::DataShare {
class ResultSetJsonFormatter final : public DistributedData::Serializable {
public:
    explicit ResultSetJsonFormatter(const std::shared_ptr<NativeRdb::ResultSet> &resultSet) : resultSet(resultSet) {}
    ~ResultSetJsonFormatter() {}
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;

private:
    bool MarshalRow(json &node, int columnCount) const;
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_RESULTSET_JSON_FORMATTER_H
