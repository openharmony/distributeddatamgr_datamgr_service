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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_DEVICE_MATRIX_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_DEVICE_MATRIX_H
#include <map>
#include <mutex>
#include "eventcenter/event.h"
#include "lru_bucket.h"
#include "metadata/matrix_meta_data.h"
#include "metadata/store_meta_data.h"
#include "utils/ref_count.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT DeviceMatrix {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    static constexpr uint16_t META_STORE_MASK = 0x0001;
    enum : int32_t {
        MATRIX_ONLINE = Event::EVT_CUSTOM,
        MATRIX_META_FINISHED,
        MATRIX_BROADCAST,
        MATRIX_BUTT
    };
    static DeviceMatrix &GetInstance();
    bool Initialize(uint32_t token, std::string storeId);
    void Online(const std::string &device, RefCount refCount = {});
    void Offline(const std::string &device);
    uint16_t OnBroadcast(const std::string &device, uint16_t code);
    void OnChanged(uint16_t code);
    void OnChanged(const StoreMetaData &metaData);
    void OnExchanged(const std::string &device, uint16_t code, bool isRemote = false);
    bool IsChangedInTerm(const StoreMetaData &metaData, uint64_t term);
    uint16_t GetCode(const StoreMetaData &metaData);
    std::pair<bool, uint16_t> GetMask(const std::string &device);
    std::pair<bool, uint16_t> GetRemoteMask(const std::string &device);
    MatrixMetaData GetMatrixMeta(const std::string &device);
    void Clear();

private:
    static constexpr uint32_t CURRENT_VERSION = 1;
    static constexpr uint32_t CURRENT_MASK = 0x000E;
    static constexpr size_t MAX_DEVICES = 16;

    DeviceMatrix();
    ~DeviceMatrix();
    DeviceMatrix(const DeviceMatrix &) = delete;
    DeviceMatrix(DeviceMatrix &&) noexcept = delete;
    DeviceMatrix &operator=(const DeviceMatrix &) = delete;
    DeviceMatrix &operator=(DeviceMatrix &&) noexcept = delete;
    static inline uint16_t SetMask(size_t index)
    {
        return 0x1 << (index + 1);
    }

    uint16_t ConvertMask(const std::string &device, uint16_t code);

    struct Mask {
        uint16_t bitset = META_STORE_MASK | CURRENT_MASK;
    };

    uint32_t tokenId_ = 0;
    std::string storeId_;
    std::mutex mutex_;
    std::map<std::string, Mask> onLines_;
    std::map<std::string, Mask> offLines_;
    std::map<std::string, Mask> remotes_;
    std::vector<std::string> maskApps_ = { "distributed_device_profile_service", "bundle_manager_service",
        "dtbhardware_manager_service" };
    LRUBucket<std::string, MatrixMetaData> versions_{ MAX_DEVICES };
    LRUBucket<uint32_t, TimePoint> changeTime_ { 64 };
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_DEVICE_MATRIX_H
