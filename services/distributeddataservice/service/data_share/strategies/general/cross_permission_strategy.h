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

#ifndef DATASHARESERVICE_WHITE_PERMISSION_STRAGETY_H
#define DATASHARESERVICE_WHITE_PERMISSION_STRAGETY_H

#include "strategy.h"
namespace OHOS::DataShare {
class CrossPermissionStrategy final : public Strategy {
public:
    explicit CrossPermissionStrategy(std::initializer_list<std::string> permissions);
    bool operator()(std::shared_ptr<Context> context) override;
private:
    std::vector<std::string> allowCrossPermissions_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_WHITE_PERMISSION_STRAGETY_H
