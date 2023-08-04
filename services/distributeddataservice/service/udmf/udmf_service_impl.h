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

#ifndef UDMF_SERVICE_IMPL_H
#define UDMF_SERVICE_IMPL_H

#include <vector>

#include "udmf_service_stub.h"

namespace OHOS {
namespace UDMF {
/*
 * UDMF server implementation
 */
class UdmfServiceImpl final : public UdmfServiceStub {
public:
    UdmfServiceImpl() = default;
    ~UdmfServiceImpl() = default;

    int32_t SetData(CustomOption &option, UnifiedData &unifiedData, std::string &key) override;
    int32_t GetData(const QueryOption &query, UnifiedData &unifiedData) override;
    int32_t GetBatchData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet) override;
    int32_t UpdateData(const QueryOption &query, UnifiedData &unifiedData) override;
    int32_t DeleteData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet) override;
    int32_t GetSummary(const QueryOption &query, Summary &summary) override;
    int32_t AddPrivilege(const QueryOption &query, Privilege &privilege) override;
    int32_t Sync(const QueryOption &query, const std::vector<std::string> &devices) override;
    int32_t OnInitialize() override;

private:
    class Factory {
    public:
        Factory();
        ~Factory();

    private:
        std::shared_ptr<UdmfServiceImpl> product_;
    };
    static Factory factory_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_SERVICE_IMPL_H
