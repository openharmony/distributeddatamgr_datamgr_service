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

#include "communication_strategy.h"
#include "log_print.h"
#include "device_manager_adapter.h"
#include "kvstore_utils.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "CommunicationStrategy"

namespace OHOS {
namespace AppDistributedKv {
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using KvStoreUtils = OHOS::DistributedKv::KvStoreUtils;
CommunicationStrategy &CommunicationStrategy::GetInstance()
{
    static CommunicationStrategy instance;
    return instance;
}

Status CommunicationStrategy::Init()
{
    return DmAdapter::GetInstance().StartWatchDeviceChange(this, {"strategy"});
}

void CommunicationStrategy::OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
                                            const AppDistributedKv::DeviceChangeType &type) const
{
    UpdateCommunicationStrategy(info, type);
}

void CommunicationStrategy::UpdateCommunicationStrategy(const AppDistributedKv::DeviceInfo &info,
                                                        const AppDistributedKv::DeviceChangeType &type) const
{
    ZLOGD("[UpdateCommunicationStrategy] to %{public}s, type:%{public}d",
          KvStoreUtils::ToBeAnonymous(info.uuid).c_str(), type);
    if (type == AppDistributedKv::DeviceChangeType::DEVICE_ONLINE) {
        strategys_.InsertOrAssign(info.uuid, true);
    } else if (type == AppDistributedKv::DeviceChangeType::DEVICE_ONREADY) {
        strategys_.Erase(info.uuid);
    } else {
        ;
    }
}

bool CommunicationStrategy::GetStrategy(const std::string &deviceId, int32_t dataLen, std::vector<LinkType> &linkTypes)
{
    if (dataLen < SWITCH_CONNECTION_THRESHOLD || !strategys_.Contains(deviceId)) {
        return false;
    }

    linkTypes.emplace_back(LINK_TYPE_WIFI_WLAN_5G);
    linkTypes.emplace_back(LINK_TYPE_WIFI_WLAN_2G);
    linkTypes.emplace_back(LINK_TYPE_WIFI_P2P);
    linkTypes.emplace_back(LINK_TYPE_BR);
    return true;
}
}  // namespace AppDistributedKv
}  // namespace OHOS