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
RadarReporter::RadarReporter(const char *eventName, BizScene scene, const char *funcName)
    : scene_(scene), eventName_(eventName), funcName_(funcName)
{
    if (funcName_ == nullptr) {
        funcName_ = UNKNOW;
    }
    if (eventName_ == nullptr) {
        eventName_ = EventName::CLOUD_SYNC_BEHAVIOR;
    }
    Report(eventName_, scene_, BizState::BEGIN, funcName_, BizStage::GENERAL_STAGE);
}

RadarReporter::~RadarReporter()
{
    Report(eventName_, scene_, BizState::END, funcName_, BizStage::GENERAL_STAGE, errCode_);
}

RadarReporter &RadarReporter::operator=(int errCode)
{
    errCode_ = errCode;
    return *this;
}

void RadarReporter::Report(const char *eventName, int scene, int state, const char *funcName, int stage, int errorCode)
{
    int stageRes = static_cast<int>(StageRes::RES_SUCCESS);
    if (errorCode != NO_ERROR) {
        stageRes = static_cast<int>(StageRes::RES_FAILED);
    }
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
        eventName,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        ORG_PKG_LABEL, ORG_PKG,
        FUNC_LABEL, funcName,
        BIZ_SCENE_LABEL, scene,
        BIZ_STAGE_LABEL, stage,
        BIZ_STATE_LABEL, state,
        STAGE_RES_LABEL, stageRes,
        ERROR_CODE_LABEL, errorCode,
        LOCAL_UUID_LABEL, AnonymousUuid(DmAdapter::GetInstance().GetLocalDevice().uuid));
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