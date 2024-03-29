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

#ifndef DATASHARESERVICE_DIV_STRAGETY_H
#define DATASHARESERVICE_DIV_STRAGETY_H

#include "context.h"
#include "strategy.h"

namespace OHOS::DataShare {
class DivStrategy : public Strategy {
public:
    DivStrategy(std::shared_ptr<Strategy> check,
                std::shared_ptr<Strategy> trueAction, std::shared_ptr<Strategy> falseAction)
        : check_(check), trueAction_(trueAction), falseAction_(falseAction)
    {
    }
    bool operator()(std::shared_ptr<Context> context) override;

private:
    std::shared_ptr<Strategy> check_;
    std::shared_ptr<Strategy> trueAction_;
    std::shared_ptr<Strategy> falseAction_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_DIV_STRAGETY_H
