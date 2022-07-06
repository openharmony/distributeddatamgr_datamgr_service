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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEVICE_STORE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEVICE_STORE_IMPL_H
#include "single_store_impl.h"
namespace OHOS::DistributedKv {
class DeviceStoreImpl : public SingleStoreImpl {
public:
    using SingleStoreImpl::SingleStoreImpl;
    ~DeviceStoreImpl() = default;

protected:
    std::vector<uint8_t> ToLocalDBKey(const Key &key) const override;
    std::vector<uint8_t> ToWholeDBKey(const Key &key) const override;
    Key ToKey(DBKey &&key) const override;
    std::vector<uint8_t> GetPrefix(const Key &prefix) const override;
    std::vector<uint8_t> GetPrefix(const DataQuery &query) const override;
    Convert GetConvert() const override;
    std::vector<uint8_t> ConvertNetwork(const Key &in, bool withLen = false) const;
    std::vector<uint8_t> ToLocal(const Key &in, bool withLen) const;

private:
    static constexpr size_t MAX_DEV_KEY_LEN = 896;
};
}
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEVICE_STORE_IMPL_H
