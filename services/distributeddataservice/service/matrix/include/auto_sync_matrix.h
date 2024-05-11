/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_AUTO_SYNC_MATRIX_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_AUTO_SYNC_MATRIX_H

#include <bitset>
#include <map>
#include <mutex>
#include "metadata/store_meta_data.h"

namespace OHOS::DistributedData {
class API_EXPORT AutoSyncMatrix {
public:
    static AutoSyncMatrix &GetInstance();
    void Initialize();
    void Online(const std::string &device);
    void Offline(const std::string &device);
    void OnChanged(const StoreMetaData &metaData);
    void OnExchanged(const std::string &device, const StoreMetaData &metaData);
    std::vector<StoreMetaData> GetChangedStore(const std::string &device);

private:
    static constexpr uint32_t MAX_SIZE = 128;
    struct Mask {
        std::bitset<MAX_SIZE> data;
        void Delete(size_t pos);
        void Set(size_t pos);
        void Reset(size_t pos);
        void Init(size_t size);
    };
    enum DataType {
        STATICS = 0,
        DYNAMICAL,
    };

    AutoSyncMatrix();
    ~AutoSyncMatrix();
    AutoSyncMatrix(const AutoSyncMatrix &) = delete;
    AutoSyncMatrix(AutoSyncMatrix &&) noexcept = delete;
    AutoSyncMatrix &operator=(const AutoSyncMatrix &) = delete;
    AutoSyncMatrix &operator=(AutoSyncMatrix &&) noexcept = delete;
    bool IsAutoSync(const StoreMetaData &meta);
    void AddStore(const StoreMetaData &meta);
    void DelStore(const StoreMetaData &meta);
    void UpdateStore(const StoreMetaData &meta);

    std::mutex mutex_;
    std::map<std::string, Mask> onlines_;
    std::map<std::string, Mask> offlines_;
    std::vector<StoreMetaData> metas_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_AUTO_SYNC_MATRIX_H