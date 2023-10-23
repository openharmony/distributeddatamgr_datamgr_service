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
#define LOG_TAG "UpdateStrategy"

#include "update_strategy.h"

#include "general/load_config_common_strategy.h"
#include "general/load_config_data_info_strategy.h"
#include "general/load_config_from_bundle_info_strategy.h"
#include "general/permission_strategy.h"
#include "general/process_single_app_user_cross_strategy.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
int64_t UpdateStrategy::Execute(
    std::shared_ptr<Context> context, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket)
{
    auto &preProcess = GetStrategy();
    if (preProcess.IsEmpty()) {
        ZLOGE("get strategy fail, maybe memory not enough");
        return -1;
    }
    if (!preProcess(context)) {
        ZLOGE("pre process fail, uri: %{public}s", DistributedData::Anonymous::Change(context->uri).c_str());
        return -1;
    }
    auto delegate = DBDelegate::Create(context->calledSourceDir, context->version, true,
        context->isEncryptDb, context->secretMetaKey);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s %{public}s", context->calledBundleName.c_str(), context->calledTableName.c_str());
        return -1;
    }
    return delegate->Update(context->calledTableName, predicate, valuesBucket);
}

SeqStrategy &UpdateStrategy::GetStrategy()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!strategies_.IsEmpty()) {
        return strategies_;
    }
    std::initializer_list<Strategy *> list = {
        new (std::nothrow)LoadConfigCommonStrategy(),
        new (std::nothrow)LoadConfigFromBundleInfoStrategy(),
        new (std::nothrow)PermissionStrategy(),
        new (std::nothrow)LoadConfigDataInfoStrategy(),
        new (std::nothrow)ProcessSingleAppUserCrossStrategy()
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