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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_DEVICE_MANAGER_ADAPTER_H
#define DISTRIBUTEDDATAMGR_DATAMGR_DEVICE_MANAGER_ADAPTER_H

#include <set>
#include <string>
#include <vector>
#include "app_device_change_listener.h"
#include "commu_types.h"
#include "concurrent_map.h"
#include "device_manager.h"
#include "device_manager_callback.h"
#include "dm_device_info.h"
#include "kv_scheduler.h"
#include "lru_bucket.h"

namespace OHOS {
namespace DistributedData {
class API_EXPORT DeviceManagerAdapter {
public:
    using DmDeviceInfo =  OHOS::DistributedHardware::DmDeviceInfo;
    using KvScheduler = OHOS::DistributedKv::KvScheduler;
    using DeviceInfo = OHOS::AppDistributedKv::DeviceInfo;
    using PipeInfo = OHOS::AppDistributedKv::PipeInfo;
    using AppDeviceChangeListener = OHOS::AppDistributedKv::AppDeviceChangeListener;
    using Status = OHOS::DistributedKv::Status;
    static DeviceManagerAdapter &GetInstance();
    void Init();
    Status StartWatchDeviceChange(const AppDeviceChangeListener *observer, const PipeInfo &pipeInfo);
    Status StopWatchDeviceChange(const AppDeviceChangeListener *observer, const PipeInfo &pipeInfo);
    DeviceInfo GetLocalDevice();
    std::vector<DeviceInfo> GetRemoteDevices();
    DeviceInfo GetDeviceInfo(const std::string &id);
    std::string GetUuidByNetworkId(const std::string &networkId);
    std::string GetUdidByNetworkId(const std::string &networkId);
    DeviceInfo GetLocalBasicInfo();
    std::string ToUUID(const std::string &id);
    static std::vector<std::string> ToUUID(const std::vector<std::string> &devices);
    static std::vector<std::string> ToUUID(std::vector<DeviceInfo> devices);
    std::string ToNetworkID(const std::string &id);
    void NotifyReadyEvent(const std::string &uuid);
    friend class DataMgrDmStateCall;

private:
    DeviceManagerAdapter();
    ~DeviceManagerAdapter();
    std::function<void()> RegDevCallback();
    bool GetDeviceInfo(const DmDeviceInfo &dmInfo, DeviceInfo &dvInfo);
    void SaveDeviceInfo(const DeviceInfo &deviceInfo, const AppDistributedKv::DeviceChangeType &type);
    void UpdateDeviceInfo();
    DeviceInfo GetDeviceInfoFromCache(const std::string &id);
    void Online(const DmDeviceInfo &info);
    void Offline(const DmDeviceInfo &info);
    void OnChanged(const DmDeviceInfo &info);
    void OnReady(const DmDeviceInfo &info);
    void TimeOut(const std::string uuid);
    std::vector<const AppDeviceChangeListener *> GetObservers();

    std::mutex devInfoMutex_ {};
    DeviceInfo localInfo_ {};
    ConcurrentMap<const AppDeviceChangeListener *, const AppDeviceChangeListener *> observers_ {};
    LRUBucket<std::string, DeviceInfo> deviceInfos_ {64};
    static constexpr size_t TIME_TASK_CAPACITY = 50;
    KvScheduler scheduler_ {TIME_TASK_CAPACITY, "dm_adapter"};
    static constexpr int32_t SYNC_TIMEOUT = 30 * 1000; // ms
    ConcurrentMap<std::string, std::string> syncTask_ {};
};
}  // namespace DistributedData
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_DATAMGR_DEVICE_MANAGER_ADAPTER_H
