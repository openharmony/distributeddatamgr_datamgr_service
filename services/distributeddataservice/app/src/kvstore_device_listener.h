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

#ifndef DISTRIBUTEDDATAMGR_KVSTORE_DEVICE_LISTENER_H
#define DISTRIBUTEDDATAMGR_KVSTORE_DEVICE_LISTENER_H
#include "app_device_change_listener.h"

namespace OHOS::DistributedKv {
using AppDeviceChangeListener = AppDistributedKv::AppDeviceChangeListener;
class KvStoreDataService;
class KvStoreDeviceListener : public AppDeviceChangeListener {
public:
    explicit KvStoreDeviceListener(KvStoreDataService &kvStoreDataService);
    void OnDeviceChanged(
        const AppDistributedKv::DeviceInfo &info, const AppDistributedKv::DeviceChangeType &type) const override;
    AppDistributedKv::ChangeLevelType GetChangeLevelType() const override;
private:
    KvStoreDataService &kvStoreDataService_;
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR_KVSTORE_DEVICE_LISTENER_H
