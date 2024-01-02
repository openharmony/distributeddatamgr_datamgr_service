/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_PROFILE_CONFIG_H
#define DISTRIBUTEDDATAMGR_PROFILE_CONFIG_H

#include <string>
#include <map>

#include <vector>
#include "data_share_profile_info.h"

namespace OHOS::DataShare {
using ProfileInfo = RdbBMSAdapter::ProfileInfo;
class DataShareProfileConfig {
public:
    static bool GetProfileInfo(const std::string &calledBundleName, int32_t currentUserId,
        std::map<std::string, ProfileInfo> &profileInfos);
};
} // namespace OHOS::DataShare
#endif // DISTRIBUTEDDATAMGR_PROFILE_CONFIG_H
