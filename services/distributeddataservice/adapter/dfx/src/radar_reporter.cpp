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

#include "radar_reporter.h"
#include "device_manager_adapter.h"
#include "hisysevent.h"

namespace OHOS {
namespace DistributedDataDfx {
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
RadarReporter::RadarReporter(const char *eventName, int32_t scene, const char *bundleName, const char *funcName)
    : eventName_(eventName), funcName_(funcName)
{
    radarParam_.scene_ = scene;
    radarParam_.bundleName_ = bundleName;
    radarParam_.res_ = RES_IDLE;
    Report(radarParam_, funcName_, BEGIN, eventName_);
}

RadarReporter::~RadarReporter()
{
    Report(radarParam_, funcName_, END, eventName_);
}

RadarReporter &RadarReporter::operator=(int32_t errCode)
{
    radarParam_.errCode_ = errCode;
    return *this;
}

void RadarReporter::Report(const RadarParam &param, const char *funcName, int32_t state, const char *eventName)
{
    int32_t res = state == BEGIN ? param.res_ : (param.errCode_ != NO_ERROR ? RES_FAILED : RES_SUCCESS);
    if (state != 0) {
        HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::DISTRIBUTED_DATAMGR, eventName,
            OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, ORG_PKG_LABEL, ORG_PKG, FUNC_LABEL, funcName,
            BIZ_SCENE_LABEL, param.scene_, BIZ_STAGE_LABEL, param.stage_, BIZ_STATE_LABEL, state, STAGE_RES_LABEL, res,
            ERROR_CODE_LABEL, param.errCode_, HOST_PKG, param.bundleName_, LOCAL_UUID_LABEL,
            AnonymousUuid(DmAdapter::GetInstance().GetLocalDevice().uuid));
    } else {
        HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::DISTRIBUTED_DATAMGR, eventName,
            OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, ORG_PKG_LABEL, ORG_PKG, FUNC_LABEL, funcName,
            BIZ_SCENE_LABEL, param.scene_, BIZ_STAGE_LABEL, param.stage_, STAGE_RES_LABEL, res, ERROR_CODE_LABEL,
            param.errCode_, HOST_PKG, param.bundleName_, LOCAL_UUID_LABEL,
            AnonymousUuid(DmAdapter::GetInstance().GetLocalDevice().uuid));
    }
    return;
}

std::string RadarReporter::AnonymousUuid(const std::string &uuid)
{
    if (uuid.length() < BASE_SIZE) {
        return DEFAULT_ANONYMOUS;
    }
    return (uuid.substr(0, HEAD_SIZE) + REPLACE_CHAIN + uuid.substr(uuid.length() - END_SIZE, END_SIZE));
}
} // namespace DistributedDataDfx
} // namespace OHOS