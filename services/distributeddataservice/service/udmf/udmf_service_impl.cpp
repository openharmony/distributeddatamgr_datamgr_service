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
#define LOG_TAG "UdmfServiceImpl"

#include "udmf_service_impl.h"

#include "iservice_registry.h"

#include "data_manager.h"
#include "lifecycle/lifecycle_manager.h"
#include "log_print.h"
#include "preprocess_utils.h"
#include "reporter.h"

namespace OHOS {
namespace UDMF {
using FeatureSystem = DistributedData::FeatureSystem;
using UdmfBehaviourMsg = OHOS::DistributedDataDfx::UdmfBehaviourMsg;
using Reporter = OHOS::DistributedDataDfx::Reporter;
__attribute__((used)) UdmfServiceImpl::Factory UdmfServiceImpl::factory_;
UdmfServiceImpl::Factory::Factory()
{
    ZLOGI("Register udmf creator!");
    FeatureSystem::GetInstance().RegisterCreator("udmf", [this]() {
        if (product_ == nullptr) {
            product_ = std::make_shared<UdmfServiceImpl>();
        }
        return product_;
    });
}

UdmfServiceImpl::Factory::~Factory()
{
    product_ = nullptr;
}

int32_t UdmfServiceImpl::SetData(CustomOption &option, UnifiedData &unifiedData, std::string &key)
{
    ZLOGD("start");
    int32_t res = E_OK;
    UdmfBehaviourMsg msg;
    auto find = UD_INTENTION_MAP.find(option.intention);
    msg.channel = find == UD_INTENTION_MAP.end() ? "invalid" : find->second;
    msg.operation = "insert";
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(option.tokenId, bundleName)) {
        msg.appId = "unknown";
        res = E_ERROR;
    } else {
        msg.appId = bundleName;
        res = DataManager::GetInstance().SaveData(option, unifiedData, key);
    }
    auto errFind = ERROR_MAP.find(res);
    msg.result = errFind == ERROR_MAP.end() ? "E_ERROR" : errFind->second;
    msg.dataType = unifiedData.GetTypes();
    msg.dataSize = unifiedData.GetSize();
    Reporter::GetInstance()->BehaviourReporter()->UDMFReport(msg);
    return res;
}

int32_t UdmfServiceImpl::GetData(const QueryOption &query, UnifiedData &unifiedData)
{
    ZLOGD("start");
    int32_t res = E_OK;
    UdmfBehaviourMsg msg;
    auto find = UD_INTENTION_MAP.find(query.intention);
    msg.channel = find == UD_INTENTION_MAP.end() ? "invalid" : find->second;
    msg.operation = "insert";
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName)) {
        msg.appId = "unknown";
        res = E_ERROR;
    } else {
        msg.appId = bundleName;
        res = DataManager::GetInstance().RetrieveData(query, unifiedData);
    }
    auto errFind = ERROR_MAP.find(res);
    msg.result = errFind == ERROR_MAP.end() ? "E_ERROR" : errFind->second;
    msg.dataType = unifiedData.GetTypes();
    msg.dataSize = unifiedData.GetSize();
    Reporter::GetInstance()->BehaviourReporter()->UDMFReport(msg);
    return res;
}

int32_t UdmfServiceImpl::GetBatchData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet)
{
    ZLOGD("start");
    return DataManager::GetInstance().RetrieveBatchData(query, unifiedDataSet);
}

int32_t UdmfServiceImpl::UpdateData(const QueryOption &query, UnifiedData &unifiedData)
{
    ZLOGD("start");
    return DataManager::GetInstance().UpdateData(query, unifiedData);
}

int32_t UdmfServiceImpl::DeleteData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet)
{
    ZLOGD("start");
    return DataManager::GetInstance().DeleteData(query, unifiedDataSet);
}

int32_t UdmfServiceImpl::GetSummary(const QueryOption &query, Summary &summary)
{
    ZLOGD("start");
    return DataManager::GetInstance().GetSummary(query, summary);
}

int32_t UdmfServiceImpl::AddPrivilege(const QueryOption &query, Privilege &privilege)
{
    ZLOGD("start");
    return DataManager::GetInstance().AddPrivilege(query, privilege);
}

int32_t UdmfServiceImpl::Sync(const QueryOption &query, const std::vector<std::string> &devices)
{
    ZLOGD("start");
    return DataManager::GetInstance().Sync(query, devices);
}

int32_t UdmfServiceImpl::OnInitialize()
{
    ZLOGD("start");
    Status status = LifeCycleManager::GetInstance().DeleteOnStart();
    if (status != E_OK) {
        ZLOGE("DeleteOnStart execute failed, status: %{public}d", status);
    }
    status = LifeCycleManager::GetInstance().DeleteOnSchedule();
    if (status != E_OK) {
        ZLOGE("ScheduleTask start failed, status: %{public}d", status);
    }
    return DistributedData::FeatureSystem::STUB_SUCCESS;
}
} // namespace UDMF
} // namespace OHOS