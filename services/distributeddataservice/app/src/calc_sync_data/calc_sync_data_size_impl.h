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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CALC_SYNC_DATA_SIZE_IMPL_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CALC_SYNC_DATA_SIZE_IMPL_H

#include "app_device_change_listener.h"
#include "concurrent_map.h"
#include "calc_sync_data_size.h"
namespace OHOS::DistributedData {
class API_EXPORT CalcSyncDataSizeImpl : public CalcSyncDataSize, public AppDistributedKv::AppDeviceChangeListener {
public:
    CalcSyncDataSizeImpl();
    virtual ~CalcSyncDataSizeImpl() = default;
    void OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
                         const AppDistributedKv::DeviceChangeType &type) const override;
    uint32_t CalcDataSize(const std::string &deviceId) override;
private:
    void CalcSyncDataSize(const std::string &deviceId) const;
    uint32_t CalcMetaDataSize(const std::string &deviceId) const;
    uint32_t CalcKvDataSize(const std::string &deviceId) const;

    mutable ConcurrentMap<std::string, uint32_t> dataSizes_;
};
}
#endif //DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CALC_SYNC_DATA_SIZE_IMPL_H
