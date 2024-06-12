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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "error_code.h"
#include "store_cache.h"
#include "udmf_service_stub.h"
#include "unified_data.h"
#include "unified_types.h"

namespace OHOS {
namespace UDMF {
/*
 * UDMF server implementation
 */
class UdmfServiceImpl final : public UdmfServiceStub {
public:
    UdmfServiceImpl();
    ~UdmfServiceImpl() = default;

    int32_t SetData(CustomOption &option, UnifiedData &unifiedData, std::string &key) override;
    int32_t GetData(const QueryOption &query, UnifiedData &unifiedData) override;
    int32_t GetBatchData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet) override;
    int32_t UpdateData(const QueryOption &query, UnifiedData &unifiedData) override;
    int32_t DeleteData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet) override;
    int32_t GetSummary(const QueryOption &query, Summary &summary) override;
    int32_t AddPrivilege(const QueryOption &query, Privilege &privilege) override;
    int32_t Sync(const QueryOption &query, const std::vector<std::string> &devices) override;
    int32_t IsRemoteData(const QueryOption &query, bool &result) override;
    int32_t SetAppShareOption(const std::string &intention, int32_t shareOption) override;
    int32_t GetAppShareOption(const std::string &intention, int32_t &shareOption) override;
    int32_t RemoveAppShareOption(const std::string &intention) override;
    int32_t OnInitialize() override;
    int32_t OnBind(const BindInfo &bindInfo) override;

private:
    int32_t SaveData(CustomOption &option, UnifiedData &unifiedData, std::string &key);
    int32_t RetrieveData(const QueryOption &query, UnifiedData &unifiedData);
    int32_t QueryDataCommon(const QueryOption &query, std::vector<UnifiedData> &dataSet, std::shared_ptr<Store> &store);
    int32_t ProcessUri(const QueryOption &query, UnifiedData &unifiedData);
    void SetRemoteUri(const QueryOption &query, std::vector<std::shared_ptr<UnifiedRecord>> &records);
    bool IsPermissionInCache(const QueryOption &query);
    bool IsReadAndKeep(const std::vector<Privilege> &privileges, const QueryOption &query);

    using StaticActs = DistributedData::StaticActs;
    class UdmfStatic : public StaticActs {
    public:
        ~UdmfStatic() override {};
        int32_t OnAppInstall(const std::string &bundleName, int32_t user, int32_t index) override;
        int32_t OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index) override;
        int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index) override;
    };
    class Factory {
    public:
        Factory();
        ~Factory();

    private:
        std::shared_ptr<UdmfServiceImpl> product_;
        std::shared_ptr<UdmfStatic> staticActs_;
    };
    static Factory factory_;
    std::map<std::string, Privilege> privilegeCache_;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_SERVICE_IMPL_H
