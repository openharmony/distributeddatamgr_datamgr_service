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
#include <vector>
#include "convertor.h"
namespace OHOS::DistributedKv {
class DeviceConvertor : public Convertor {
public:
    std::vector<uint8_t> ToLocalDBKey(const Key &key) const override;
    std::vector<uint8_t> ToWholeDBKey(const Key &key) const override;
    Key ToKey(DBKey &&key, std::string &deviceId) const override;
    std::vector<uint8_t> GetPrefix(const Key &prefix) const override;
    std::vector<uint8_t> GetPrefix(const DataQuery &query) const override;

protected:
    std::string GetRealKey(const std::string &key, const DataQuery &query) const override;

private:
    static constexpr size_t MAX_DEV_KEY_LEN = 896;
    std::vector<uint8_t> ConvertNetwork(const Key &in, bool withLen = false) const;
    std::vector<uint8_t> ToLocal(const Key &in, bool withLen) const;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEVICE_STORE_IMPL_H
