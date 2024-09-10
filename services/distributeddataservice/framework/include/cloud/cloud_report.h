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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_REPORT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_REPORT_H

#include "store/general_value.h"
#include "visibility.h"

namespace OHOS::DistributedData {
class API_EXPORT CloudReport {
public:
    using ReportParam = DistributedData::ReportParam;
    API_EXPORT static CloudReport *GetInstance();
    API_EXPORT static bool RegisterCloudReportInstance(CloudReport *instance);
    virtual std::string GetPrepareTraceId(int32_t userId, const std::string &bundleName);
    virtual std::string GetRequestTraceId(int32_t userId, const std::string &bundleName);
    virtual bool Report(const ReportParam &reportParam);

private:
    static CloudReport *instance_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_REPORT_H