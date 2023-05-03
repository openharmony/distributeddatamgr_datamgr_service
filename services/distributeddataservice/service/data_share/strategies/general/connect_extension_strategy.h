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

#ifndef DATASHARESERVICE_CONNECT_EXTENSION_STRAGETY_H
#define DATASHARESERVICE_CONNECT_EXTENSION_STRAGETY_H

#include "block_data.h"
#include "strategy.h"
namespace OHOS::DataShare {
class ConnectExtensionStrategy final : public Strategy {
public:
    ConnectExtensionStrategy();
    bool operator()(std::shared_ptr<Context> context) override;
    static bool Execute(std::shared_ptr<Context> context, std::function<bool()> isFinished, int maxWaitTimeMs = 2000);
private:
    inline bool Connect(std::shared_ptr<Context> context);
    std::mutex mutex_;
    BlockData<bool> data_;
};
} // namespace OHOS::DataShare
#endif
