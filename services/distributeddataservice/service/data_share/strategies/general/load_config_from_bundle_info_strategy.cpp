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
#include "load_config_from_bundle_info_strategy.h"

#include "check_is_data_proxy_strategy.h"
#include "data_proxy/load_config_from_data_proxy_node_strategy.h"
#include "data_share/load_config_from_data_share_bundle_info_strategy.h"

namespace OHOS::DataShare {
LoadConfigFromBundleInfoStrategy::LoadConfigFromBundleInfoStrategy()
    : DivStrategy(std::make_shared<CheckIsDataProxyStrategy>(), std::make_shared<LoadConfigFromDataProxyNodeStrategy>(),
          std::make_shared<LoadConfigFromDataShareBundleInfoStrategy>())
{
}
} // namespace OHOS::DataShare