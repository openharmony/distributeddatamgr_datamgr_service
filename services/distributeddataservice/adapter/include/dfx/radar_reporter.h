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
#include "dfx_types.h"

namespace OHOS {
namespace DistributedDataDfx {
class RadarReporter {
public:
    KVSTORE_API RadarReporter(const char *eventName, BizScene scene, const char *funcName);
    KVSTORE_API ~RadarReporter();
    KVSTORE_API RadarReporter &operator=(int errCode);
    KVSTORE_API static void Report(
        const char *eventName, int scene, int state, const char *funcName, int stage = 1, int errorCode = 0);

private:
    static std::string AnonymousUuid(const std::string &uuid);
    int errCode_{ 0 };
    BizScene scene_;
    const char *eventName_;
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
    static constexpr const char *UNKNOW = "UNKNOW";
    static constexpr const char *REPLACE_CHAIN = "**";
    static constexpr const char *DEFAULT_ANONYMOUS = "************";
    static constexpr const int32_t NO_ERROR = 0;
    static constexpr const int32_t HEAD_SIZE = 5;
    static constexpr const int32_t END_SIZE = 5;
    static constexpr const int32_t BASE_SIZE = 12;
};
} // namespace DistributedDataDfx
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_RADAR_REPORTER_H