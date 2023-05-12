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
#define LOG_TAG "QueryStrategy"

#include "query_strategy.h"

#include "general/load_config_common_strategy.h"
#include "general/load_config_data_info_strategy.h"
#include "general/load_config_from_bundle_info_strategy.h"
#include "general/permission_strategy.h"
#include "general/process_single_app_user_cross_strategy.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
std::shared_ptr<DataShareResultSet> QueryStrategy::Execute(
    std::shared_ptr<Context> context, const DataSharePredicates &predicates,
    const std::vector<std::string> &columns, int &errCode)
{
    auto preProcess = GetStrategy();
    if (preProcess == nullptr) {
        ZLOGE("get strategy fail, maybe memory not enough");
        return nullptr;
    }
    if (!(*preProcess)(context)) {
        errCode = context->errCode;
        ZLOGE("pre process fail, uri: %{public}s", DistributedData::Anonymous::Change(context->uri).c_str());
        return nullptr;
    }
    auto delegate = DBDelegate::Create(context->calledSourceDir, context->version);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s %{public}s", context->calledBundleName.c_str(), context->calledTableName.c_str());
        return nullptr;
    }
    return delegate->Query(context->calledTableName, predicates, columns, errCode);
}

Strategy *QueryStrategy::GetStrategy()
{
    static std::mutex mutex;
    static SeqStrategy strategies;
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (!strategies.IsEmpty()) {
        return &strategies;
    }
    std::initializer_list<Strategy *> list = {
        new (std::nothrow)LoadConfigCommonStrategy(),
        new (std::nothrow)LoadConfigFromBundleInfoStrategy(),
        new (std::nothrow)PermissionStrategy(),
        new (std::nothrow)LoadConfigDataInfoStrategy(),
        new (std::nothrow)ProcessSingleAppUserCrossStrategy()
    };
    auto ret = strategies.Init(list);
    if (!ret) {
        std::for_each(list.begin(), list.end(), [](Strategy *item) {
            delete item;
        });
        return nullptr;
    }
    return &strategies;
}
} // namespace OHOS::DataShare