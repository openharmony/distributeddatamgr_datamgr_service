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
#include "executor_pool.h"
#include "lru_bucket.h"
#include "matrix_event.h"
#include "metadata/matrix_meta_data.h"
#include "metadata/store_meta_data.h"
#include "utils/ref_count.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT DeviceMatrix {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Origin = OHOS::DistributedData::MatrixMetaData::Origin;
    static constexpr uint16_t META_STORE_MASK = 0x0001;
    static constexpr uint16_t INVALID_HIGH = 0xFFF0;
    static constexpr uint16_t INVALID_LEVEL = 0xFFFF;
    static constexpr uint16_t INVALID_LENGTH = 0;
    static constexpr uint32_t INVALID_VALUE = 0xFFFFFFFF;
    static constexpr uint16_t INVALID_MASK = 0;
    enum : int32_t {
        MATRIX_ONLINE = Event::EVT_CUSTOM,
        MATRIX_META_FINISHED,
        MATRIX_BROADCAST,
        MATRIX_BUTT
    };
    enum LevelType : int32_t {
        STATICS = 0,
        DYNAMIC,
        BUTT,
    };
    enum ChangeType : int32_t {
        CHANGE_LOCAL = 0,
        CHANGE_REMOTE,
        CHANGE_ALL,
        CHANGE_BUTT
    };
    struct DataLevel {
        uint16_t dynamic = INVALID_LEVEL;
        uint16_t statics = INVALID_LEVEL;
        uint32_t switches = INVALID_VALUE;
        uint16_t switchesLen = INVALID_LENGTH;
        bool IsValid() const;
    };
    
    static DeviceMatrix &GetInstance();
    bool Initialize(uint32_t token, std::string storeId);
    void Online(const std::string &device, RefCount refCount = {});
    void Offline(const std::string &device);
    std::pair<uint16_t, uint16_t> OnBroadcast(const std::string &device, const DataLevel &dataLevel);
    void Broadcast(const DataLevel &dataLevel);
    void OnChanged(uint16_t code, LevelType type = LevelType::DYNAMIC);
    void OnChanged(const StoreMetaData &metaData);
    void OnExchanged(const std::string &device, uint16_t code,
        LevelType type = LevelType::DYNAMIC, ChangeType changeType = ChangeType::CHANGE_ALL);
    void OnExchanged(const std::string &device,
        const StoreMetaData &metaData, ChangeType type = ChangeType::CHANGE_ALL);
    void Clear();
    void RegRemoteChange(std::function<void(const std::string &, uint16_t)> observer);
    void SetExecutor(std::shared_ptr<ExecutorPool> executors);
    uint16_t GetCode(const StoreMetaData &metaData);
    std::pair<bool, uint16_t> GetMask(const std::string &device, LevelType type = LevelType::DYNAMIC);
    std::pair<bool, uint16_t> GetRemoteMask(const std::string &device, LevelType type = LevelType::DYNAMIC);
    std::pair<bool, uint16_t> GetRecvLevel(const std::string &device, LevelType type = LevelType::DYNAMIC);
    std::pair<bool, uint16_t> GetConsLevel(const std::string &device, LevelType type = LevelType::DYNAMIC);
    std::pair<bool, bool> IsConsistent(const std::string &device);
    std::pair<bool, MatrixMetaData> GetMatrixMeta(const std::string &device, bool IsConsistent = false);
    void SetMatrixMeta(const MatrixMetaData &meta, bool IsConsistent = false);
    std::map<std::string, uint16_t> GetRemoteDynamicMask();
    bool IsDynamic(const StoreMetaData &metaData);
    bool IsStatics(const StoreMetaData &metaData);
    bool IsSupportMatrix();

private:
    static constexpr uint32_t RESET_MASK_DELAY = 10; // min
    static constexpr uint32_t CURRENT_VERSION = 3;
    static constexpr uint16_t OLD_MASK = 0x000E;
    static constexpr uint16_t CURRENT_DYNAMIC_MASK = 0x0006;
    static constexpr uint16_t CURRENT_STATICS_MASK = 0x0003;
    static constexpr size_t MAX_DEVICES = 64;

    DeviceMatrix();
    ~DeviceMatrix();
    DeviceMatrix(const DeviceMatrix &) = delete;
    DeviceMatrix(DeviceMatrix &&) noexcept = delete;
    DeviceMatrix &operator=(const DeviceMatrix &) = delete;
    DeviceMatrix &operator=(DeviceMatrix &&) noexcept = delete;
    static inline uint16_t SetMask(size_t index)
    {
        return 0x1 << index;
    }
    static inline uint16_t Low(uint16_t data)
    {
        return data & 0x000F;
    }
    static inline uint16_t High(uint16_t data)
    {
        return data & 0xFFF0;
    }
    static inline uint16_t Merge(uint16_t high, uint16_t low)
    {
        return High(high) | Low(low);
    }
    static inline uint16_t ResetMask(uint16_t data)
    {
        return data &= 0xFFF0;
    }
    struct Mask {
        uint16_t dynamic = META_STORE_MASK | OLD_MASK; // META_STORE_MASK | CURRENT_DYNAMIC_MASK
        uint16_t statics = CURRENT_STATICS_MASK;
        void SetDynamicMask(uint16_t mask);
        void SetStaticsMask(uint16_t mask);
    };
    using TaskId = ExecutorPool::TaskId;
    using Task = ExecutorPool::Task;

    void UpdateMask(Mask &mask);
    void UpdateConsistentMeta(const std::string &device, const Mask &remote);
    void SaveSwitches(const std::string &device, const DataLevel &dataLevel);
    void UpdateRemoteMeta(const std::string &device, Mask &mask);
    Task GenResetTask();
    std::pair<uint16_t, uint16_t> ConvertMask(const std::string &device, const DataLevel &dataLevel);
    uint16_t ConvertDynamic(const MatrixMetaData &meta, uint16_t mask);
    uint16_t ConvertStatics(const MatrixMetaData &meta, uint16_t mask);
    MatrixMetaData GetMatrixInfo(const std::string &device);
    static inline uint16_t ConvertIndex(uint16_t code);

    MatrixEvent::MatrixData lastest_;
    std::mutex taskMutex_;
    std::shared_ptr<ExecutorPool> executors_;
    TaskId task_ = ExecutorPool::INVALID_TASK_ID;

    bool isSupportBroadcast_ = true;
    uint32_t tokenId_ = 0;
    std::string storeId_;
    std::mutex mutex_;
    std::map<std::string, Mask> onLines_;
    std::map<std::string, Mask> offLines_;
    std::map<std::string, Mask> remotes_;
    std::vector<std::string> dynamicApps_;
    std::vector<std::string> staticsApps_;
    std::function<void(const std::string &, uint16_t)> observer_;
    LRUBucket<std::string, MatrixMetaData> matrixs_{ MAX_DEVICES };
    LRUBucket<std::string, MatrixMetaData> versions_{ MAX_DEVICES };
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_DEVICE_MATRIX_H
