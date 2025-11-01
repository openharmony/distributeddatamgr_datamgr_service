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

#ifndef UDMF_DELAY_DATA_PREPARE_CONTAINER_H
#define UDMF_DELAY_DATA_PREPARE_CONTAINER_H

#include "block_data.h"
#include "udmf_notifier_proxy.h"
#include "unified_data.h"

namespace OHOS {
namespace UDMF {
struct BlockDelayData {
    uint32_t tokenId {0};
    std::shared_ptr<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>> blockData;
};

class DelayDataPrepareContainer {
public:
    static DelayDataPrepareContainer &GetInstance();
    // dataLoadCallback_ part
    bool HandleDelayLoad(const QueryOption &query, UnifiedData &unifiedData);
    void RegisterDataLoadCallback(const std::string &key, sptr<UdmfNotifierProxy> callback);
    int QueryDataLoadCallbackSize();
    bool ExecDataLoadCallback(const std::string &key, const DataLoadInfo &info);
    void ExecAllDataLoadCallback();

    // blockDelayDataCache_ part
    bool QueryBlockDelayData(const std::string &key, BlockDelayData &getDataInfo);

private:
    DelayDataPrepareContainer() = default;
    ~DelayDataPrepareContainer() = default;
    DelayDataPrepareContainer(const DelayDataPrepareContainer &obj) = delete;
    DelayDataPrepareContainer &operator=(const DelayDataPrepareContainer &obj) = delete;

    std::mutex dataLoadMutex_;
    std::map<std::string, sptr<UdmfNotifierProxy>> dataLoadCallback_ {};
    std::map<std::string, BlockDelayData> blockDelayDataCache_ {};
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_DELAY_DATA_PREPARE_CONTAINER_H