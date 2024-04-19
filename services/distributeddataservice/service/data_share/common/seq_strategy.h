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

#ifndef DATASHARESERVICE_SEQ_STRAGETY_H
#define DATASHARESERVICE_SEQ_STRAGETY_H

#include <list>

#include "context.h"
#include "strategy.h"

namespace OHOS::DataShare {
class SeqStrategy : public Strategy {
public:
    bool IsEmpty();
    bool Init(std::initializer_list<Strategy *> strategies);
    bool operator()(std::shared_ptr<Context> context) override;

private:
    std::list<std::shared_ptr<Strategy>> actions_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_SEQ_STRAGETY_H
