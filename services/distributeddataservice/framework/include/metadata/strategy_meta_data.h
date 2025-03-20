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

#ifndef DISTRIBUTEDDATAMGR_STRATEGY_META_DATA_H
#define DISTRIBUTEDDATAMGR_STRATEGY_META_DATA_H
#include "serializable/serializable.h"
#include "metadata/capability_range.h"
namespace OHOS::DistributedData {
struct API_EXPORT StrategyMeta final : public Serializable {
    std::string devId;
    std::string userId;
    std::string bundleName;
    int32_t instanceId = 0;
    std::string storeId;
    bool capabilityEnabled = false;
    CapabilityRange capabilityRange;

    API_EXPORT StrategyMeta(const std::string &devId, const std::string &userId, const std::string &bundleName,
        const std::string &storeId);
    API_EXPORT ~StrategyMeta() {};
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
    API_EXPORT bool IsEffect() const;
    API_EXPORT std::string GetKey();
    API_EXPORT static std::string GetPrefix(const std::initializer_list<std::string> &fields);

private:
    static constexpr const char *PREFIX = "StrategyMetaData";
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_STRATEGY_META_DATA_H
