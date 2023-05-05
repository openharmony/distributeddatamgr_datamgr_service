/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "GetDataStrategy"

#include "get_data_strategy.h"

#include "general/load_config_common_strategy.h"
#include "general/permission_strategy.h"
#include "data_proxy/load_config_from_data_proxy_node_strategy.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
Data GetDataStrategy::Execute(std::shared_ptr<Context> context)
{
    auto preProcess = GetStrategy();
    if (preProcess == nullptr) {
        ZLOGE("get strategy fail, maybe memory not enough");
        return Data();
    }
    if (!(*preProcess)(context)) {
        ZLOGE("pre process fail, uri: %{public}s", DistributedData::Anonymous::Change(context->uri).c_str());
        return Data();
    }
    std::vector<PublishedData> queryResult = PublishedData::Query(context->calledBundleName);
    return Convert(queryResult);
}

Strategy *GetDataStrategy::GetStrategy()
{
    static std::mutex mutex;
    static SeqStrategy strategies;
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (!strategies.IsEmpty()) {
        return &strategies;
    }
    std::initializer_list<Strategy *> list = {

        new (std::nothrow) LoadConfigCommonStrategy(),
        new (std::nothrow) LoadConfigFromDataProxyNodeStrategy(),
        new (std::nothrow) PermissionStrategy()
    };
    auto ret = strategies.Init(list);
    if (!ret) {
        return nullptr;
    }
    return &strategies;
}

Data GetDataStrategy::Convert(std::vector<PublishedData> datas)
{
    Data data;
    data.version_ = -1;
    for (auto &item : datas) {
        if (item.value.version > data.version_) {
            data.version_ = item.value.version;
        }
        data.datas_.emplace_back(item.value.key, item.value.subscriberId, item.value.value);
    }
    return data;
}
} // namespace OHOS::DataShare