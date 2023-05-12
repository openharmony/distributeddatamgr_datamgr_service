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

#ifndef DATASHARESERVICE_PERMISSION_STRAGETY_H
#define DATASHARESERVICE_PERMISSION_STRAGETY_H

#include "strategy.h"
namespace OHOS::DataShare {
class PermissionStrategy final : public Strategy {
public:
    PermissionStrategy() = default;
    bool operator()(std::shared_ptr<Context> context) override;
};
} // namespace OHOS::DataShare
#endif
