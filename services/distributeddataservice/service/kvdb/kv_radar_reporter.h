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

#ifndef KV_RADAR_REPORTER_H
#define KV_RADAR_REPORTER_H

#include "hisysevent.h"

namespace OHOS::DistributedKv {
enum BizScene {
    ONLINE_DEVICE_SYNC = 1,
    BROADCAST_DEVICE_SYNC = 2,
    STANDARD_DEVICE_SYNC = 3,
};

enum StandardStage {
    ADD_SYNC_TASK = 1,
    CHECK_DEVICE_ONLINE = 2,
    STANDARD_META_SYNC = 3,
    OPEN_STORE = 4,
    START_SYNC = 5,
    FINISH_SYNC = 6,
};

enum BroadcastStage {
    SEND_BROADCAST = 1,
};

enum OnlineStage {
    ONLINE_META_SYNC = 1,
    ONLINE_META_COMPLETE = 2,
};

enum StageRes {
    RADAR_START = 0,
    RADAR_SUCCESS = 1,
    RADAR_FAILED = 2,
    RADAR_CANCEL = 3,
};

enum BizState {
    START = 1,
    END = 2,
};

constexpr char DOMAIN[] = "DISTDATAMGR";
constexpr const char* EVENT_NAME = "DISTRIBUTED_KV_STORE_BEHAVIOR";
constexpr HiviewDFX::HiSysEvent::EventType TYPE = HiviewDFX::HiSysEvent::EventType::BEHAVIOR;
constexpr const char* ORG_PKG = "distributeddata";
constexpr const char* ERROR_CODE = "ERROR_CODE";
constexpr const char* BIZ_STATE = "BIZ_STATE";
constexpr const char* SYNC_ID = "SYNC_ID";
constexpr const char* SYNC_STORE_ID = "SYNC_STORE_ID";
constexpr const char* SYNC_APP_ID = "SYNC_APP_ID";
constexpr const char* OS_TYPE = "OS_TYPE";

#define RADAR_REPORT(bizScene, bizStage, stageRes, ...)                                            \
({                                                                                                 \
    HiSysEventWrite(DistributedKv::DOMAIN, DistributedKv::EVENT_NAME, DistributedKv::TYPE,         \
        "ORG_PKG", DistributedKv::ORG_PKG, "FUNC", __FUNCTION__,                                   \
        "BIZ_SCENE", bizScene, "BIZ_STAGE", bizStage, "STAGE_RES", stageRes, ##__VA_ARGS__);       \
})
} // namespace OHOS::DistributedKv
#endif // KV_RADAR_REPORTER_H