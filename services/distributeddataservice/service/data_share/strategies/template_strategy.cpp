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
#define LOG_TAG "TemplateStrategy"

#include "template_strategy.h"

#include "data_proxy/load_config_from_data_proxy_node_strategy.h"
#include "datashare_errno.h"
#include "general/load_config_common_strategy.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
int32_t TemplateStrategy::Execute(std::shared_ptr<Context> context, std::function<bool()> process)
{
    auto &preProcess = GetStrategy();
    if (preProcess.IsEmpty()) {
        ZLOGE("get strategy fail, maybe memory not enough");
        return E_ERROR;
    }

    if (!preProcess(context)) {
        ZLOGE("pre process fail, uri: %{public}s", DistributedData::Anonymous::Change(context->uri).c_str());
        return context->errCode;
    }
    return process();
}

SeqStrategy &TemplateStrategy::GetStrategy()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!strategies_.IsEmpty()) {
        return strategies_;
    }
    std::initializer_list<Strategy *> list = {
        new (std::nothrow) LoadConfigCommonStrategy(),
        new (std::nothrow) LoadConfigFromDataProxyNodeStrategy()
    };
    auto ret = strategies_.Init(list);
    if (!ret) {
        std::for_each(list.begin(), list.end(), [](Strategy *item) {
            delete item;
        });
        return strategies_;
    }
    return strategies_;
}
} // namespace OHOS::DataShare