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

#ifndef DISTRIBUTEDDATAMGR_RADAR_REPORTER_H
#define DISTRIBUTEDDATAMGR_RADAR_REPORTER_H
#include <stdint.h>
#include <string>

#include "visibility.h"

namespace OHOS {
namespace DistributedDataDfx {

enum BizScene {
    // cloud sync
    CLOUD_SYNC = 1,

    // cloud share
    SHARE = 1,
    UNSHARE = 2,
    CONFIRM_INVITATION = 3,
    CHANGE_CONFIRMATION = 4,
    CHANGE_PRIVILEGE = 5,
    EXIT_SHARING = 6,
};

enum BizStage {
    // cloud share
    GENERAL_STAGE = 1,

    // cloud sync
    TRIGGER_SYNC = 1,
    CHECK_SYNC_CONDITION = 2,
    START_SYNC = 3,
    FINISH_SYNC = 4,
};

enum BizState {
    BEGIN = 1,
    END = 2,
};

enum StageRes {
    RES_IDLE = 0,
    RES_SUCCESS = 1,
    RES_FAILED = 2,
    RES_CANCELLED = 3,
    RES_UNKNOWN = 4,
};

struct EventName {
    static constexpr const char *CLOUD_SYNC_BEHAVIOR = "DISTRIBUTED_CLOUD_SYNC_BEHAVIOR";
    static constexpr const char *CLOUD_SHARING_BEHAVIOR = "DISTRIBUTED_CLOUD_SHARE_BEHAVIOR";
};

struct RadarParam {
    const char *bundleName_ = "";
    int32_t scene_ = CLOUD_SYNC;
    int32_t stage_ = GENERAL_STAGE;
    uint64_t syncId_ = 0;
    int32_t triggerMode_ = 0;
    int32_t errCode_ = 0;
    uint64_t changeCount = 0;
    int32_t res_ = RES_SUCCESS;
};

class RadarReporter {
public:
    API_EXPORT RadarReporter(const char *eventName, int32_t scene, const char *bundleName, const char *funcName);
    API_EXPORT ~RadarReporter();
    API_EXPORT RadarReporter &operator=(int32_t errCode);
    API_EXPORT static void Report(const RadarParam &param, const char *funcName, int32_t state = 0,
        const char *eventName = EventName::CLOUD_SYNC_BEHAVIOR);

private:
    static std::string AnonymousUuid(const std::string &uuid);
    RadarParam radarParam_;
    const char *eventName_ = EventName::CLOUD_SYNC_BEHAVIOR;
    const char *funcName_;

    static constexpr const char *ORG_PKG_LABEL = "ORG_PKG";
    static constexpr const char *ORG_PKG = "distributddata";
    static constexpr const char *FUNC_LABEL = "FUNC";
    static constexpr const char *BIZ_SCENE_LABEL = "BIZ_SCENE";
    static constexpr const char *BIZ_STATE_LABEL = "BIZ_STATE";
    static constexpr const char *BIZ_STAGE_LABEL = "BIZ_STAGE";
    static constexpr const char *STAGE_RES_LABEL = "STAGE_RES";
    static constexpr const char *ERROR_CODE_LABEL = "ERROR_CODE";
    static constexpr const char *LOCAL_UUID_LABEL = "LOCAL_UUID";
    static constexpr const char *HOST_PKG = "HOST_PKG";
    static constexpr const char *UNKNOW = "UNKNOW";
    static constexpr const char *REPLACE_CHAIN = "**";
    static constexpr const char *DEFAULT_ANONYMOUS = "************";
    static constexpr const char *CONCURRENT_ID = "CONCURRENT_ID";
    static constexpr const char *TRIGGER_MODE = "TRIGGER_MODE";
    static constexpr const char *WATER_VERSION = "WATER_VERSION";
    static constexpr const int32_t NO_ERROR = 0;
    static constexpr const int32_t HEAD_SIZE = 5;
    static constexpr const int32_t END_SIZE = 5;
    static constexpr const int32_t BASE_SIZE = 12;
};
} // namespace DistributedDataDfx
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_RADAR_REPORTER_H