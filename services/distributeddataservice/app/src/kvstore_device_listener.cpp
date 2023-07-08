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

#define LOG_TAG "KvStoreDeviceListener"
#include "kvstore_device_listener.h"

#include "kvstore_data_service.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
void KvStoreDeviceListener::OnDeviceChanged(
    const AppDistributedKv::DeviceInfo &info, const AppDistributedKv::DeviceChangeType &type) const
{
    switch (type) {
        case AppDistributedKv::DeviceChangeType::DEVICE_ONLINE:
            kvStoreDataService_.SetCompatibleIdentify(info);
            kvStoreDataService_.OnDeviceOnline(info);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_ONREADY:
            kvStoreDataService_.OnDeviceOnReady(info);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_NET_AVAILABLE:
            kvStoreDataService_.OnNetworkOnline();
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_NET_UNAVAILABLE:
            kvStoreDataService_.OnNetworkOffline();
            break;
        default:
            break;
    }
    ZLOGI("device is %{public}d", type);
}

KvStoreDeviceListener::KvStoreDeviceListener(KvStoreDataService &kvStoreDataService)
    : kvStoreDataService_(kvStoreDataService)
{
}

AppDistributedKv::ChangeLevelType KvStoreDeviceListener::GetChangeLevelType() const
{
    return AppDistributedKv::ChangeLevelType::MIN;
}
} // namespace OHOS::DistributedKv
