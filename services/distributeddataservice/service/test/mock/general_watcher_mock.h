/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTEDDATA_SERVICE_TEST_GENERAL_WATCH_MOCK_H
#define OHOS_DISTRIBUTEDDATA_SERVICE_TEST_GENERAL_WATCH_MOCK_H
#include "rdb_query.h"
#include "store/general_watcher.h"

namespace OHOS::DistributedData {
class MockQuery : public DistributedRdb::RdbQuery {
public:
    ~MockQuery() = default;
    static constexpr uint64_t TYPE_ID = 0x20000001;
    std::vector<std::string> tables_;
    bool lastResult = false;
    bool IsEqual(uint64_t tid) override;

    std::vector<std::string> GetTables() override;
    const std::string GetStatement();
    void MakeRemoteQuery(const std::string &devices, const std::string &sql, DistributedData::Values &&args);
    void MakeQuery(const DistributedRdb::PredicatesMemo &predicates);
};

class MockGeneralWatcher : public DistributedData::GeneralWatcher {
public:
    int32_t OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values) override;

    int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) override;
};
} // namespace OHOS::DistributedData
#endif