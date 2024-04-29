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

#ifndef DATASHARESERVICE_DATA_SHARE_SILENT_CONFIG_H
#define DATASHARESERVICE_DATA_SHARE_SILENT_CONFIG_H

#include <concurrent_map.h>
#include <map>
#include <string>

#include "bundle_mgr_proxy.h"
#include "context.h"
#include "data_share_profile_config.h"

namespace OHOS::DataShare {
class DataShareSilentConfig {
public:
    bool EnableSilentProxy(uint32_t callerTokenId, const std::string &originUriStr, bool enable);
    bool IsSilentProxyEnable(uint32_t callerTokenId, int32_t currentUserId,
        const std::string &calledBundleName, const std::string &originUriStr);

private:
    ConcurrentMap<uint32_t, std::map<std::string, bool>> enableSilentUris_;
    int CheckExistEnableSilentUris(uint32_t callerTokenId, const std::string &uri, bool &isEnable);
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_DATA_SHARE_SILENT_CONFIG_H
