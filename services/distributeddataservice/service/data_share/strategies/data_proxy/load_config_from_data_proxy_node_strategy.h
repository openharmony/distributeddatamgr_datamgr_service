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

#ifndef DATASHARESERVICE_LOAD_CONFIG_FROM_DATA_PROXY_NODE_STRAGETY_H
#define DATASHARESERVICE_LOAD_CONFIG_FROM_DATA_PROXY_NODE_STRAGETY_H

#include "data_share_profile_info.h"
#include "strategy.h"

namespace OHOS::DataShare {
using namespace OHOS::RdbBMSAdapter;
class LoadConfigFromDataProxyNodeStrategy final : public Strategy {
public:
    LoadConfigFromDataProxyNodeStrategy() = default;
    bool operator()(std::shared_ptr<Context> context) override;

private:
    enum PATH_PARAMS : int32_t
    {
        STORE_NAME = 0,
        TABLE_NAME,
        PARAM_SIZE
    };
    bool LoadConfigFromUri(std::shared_ptr<Context> context);
    bool GetContextInfoFromDataProperties(
        const DataProperties &properties, const std::string &moduleName, std::shared_ptr<Context> context);
};
} // namespace OHOS::DataShare
#endif
