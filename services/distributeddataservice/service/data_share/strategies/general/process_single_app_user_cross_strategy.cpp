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
#define LOG_TAG "ProcessUserCrossStrategy"
#include "process_single_app_user_cross_strategy.h"

#include "bundlemgr/bundle_mgr_client.h"
#include "check_is_single_app_strategy.h"
#include "empty_strategy.h"
#include "log_print.h"

namespace OHOS::DataShare {
ProcessSingleAppUserCrossStrategy::ProcessSingleAppUserCrossStrategy()
    : DivStrategy(std::make_shared<CheckIsSingleAppStrategy>(), std::make_shared<ProcessUserCrossStrategy>(),
          std::make_shared<EmptyStrategy>())
{
}

bool ProcessUserCrossStrategy::operator()(std::shared_ptr<Context> context)
{
    if (context->accessSystemMode == UNDEFINED) {
        ZLOGE("single app must config user cross mode, please check it, bundleName: %{public}s",
            context->calledBundleName.c_str());
        return false;
    }
    ZLOGE("hanlu enter %{public}d", context->accessSystemMode);
    if (context->accessSystemMode == USER_SINGLE_MODE) {
        ZLOGE("hanlu table userid %{public}d", context->currentUserId);
        context->calledTableName.append("_").append(std::to_string(context->currentUserId));
        ZLOGE("hanlu table userid %{public}s", context->calledTableName.c_str());
        return true;
    }
    return true;
}
} // namespace OHOS::DataShare