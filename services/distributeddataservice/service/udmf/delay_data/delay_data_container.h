/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef UDMF_DELAY_DATA_CONTAINER_H
#define UDMF_DELAY_DATA_CONTAINER_H

#include "block_data.h"
#include "concurrent_map.h"
#include "udmf_notifier_proxy.h"
#include "unified_data.h"

namespace OHOS {
namespace UDMF {

struct BlockDelayData {
    uint32_t tokenId {0};
    std::shared_ptr<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>> blockData;
};

class DelayDataContainer {
public:
    static DelayDataContainer &GetInstance();
    // dataLoadCallback_ part
    bool HandleDelayLoad(const QueryOption &query, UnifiedData &unifiedData, int32_t &res);
    void RegisterDataLoadCallback(const std::string &key, sptr<UdmfNotifierProxy> callback);
    bool ExecDataLoadCallback(const std::string &key, const DataLoadInfo &info);
    void ExecAllDataLoadCallback();

    // delayDataCallback_ part
    void RegisterDelayDataCallback(const std::string &key, const DelayGetDataInfo &info);
    bool HandleDelayDataCallback(const std::string &key, const UnifiedData &unifiedData);
    bool QueryDelayGetDataInfo(const std::string &key, DelayGetDataInfo &info);

    // blockDelayDataCache_ part
    bool QueryBlockDelayData(const std::string &key, BlockDelayData &getDataInfo);

    // delayDragDeviceInfo_ part
    void SaveDelayDragDeviceInfo(std::string &deviceId);
    std::vector<std::string> QueryDelayDragDeviceInfo();
    void ClearDelayDragDeviceInfo();

    // delayAcceptableInfos_ part
    void SaveDelayAcceptableInfo(const std::string &key, const DataLoadInfo &info);
    bool QueryDelayAcceptableInfo(const std::string &key, DataLoadInfo &info);
private:
    DelayDataContainer() = default;
    ~DelayDataContainer() = default;
    DelayDataContainer(const DelayDataContainer &obj) = delete;
    DelayDataContainer &operator=(const DelayDataContainer &obj) = delete;

    ConcurrentMap<std::string, sptr<UdmfNotifierProxy>> dataLoadCallback_ {};
    ConcurrentMap<std::string, DelayGetDataInfo> delayDataCallback_ {};
    ConcurrentMap<std::string, BlockDelayData> blockDelayDataCache_ {};
    std::vector<std::string> delayDragDeviceInfo_ {};
    ConcurrentMap<std::string, DataLoadInfo> delayAcceptableInfos_ {};
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_DELAY_DATA_CONTAINER_H