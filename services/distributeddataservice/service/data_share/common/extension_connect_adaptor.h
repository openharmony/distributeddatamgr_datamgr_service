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

#ifndef DATASHARESERVICE_EXTENSION_CONNECT_ADAPTOR_H
#define DATASHARESERVICE_EXTENSION_CONNECT_ADAPTOR_H

#include "ability_connect_callback_interface.h"
#include "block_data.h"
#include "context.h"
namespace OHOS::DataShare {
class ExtensionConnectAdaptor {
public:
    static bool TryAndWait(std::shared_ptr<Context> context, int maxWaitTime = 2);
    ExtensionConnectAdaptor();

private:
    bool Connect(std::shared_ptr<Context> context);
    void Disconnect();
    bool DoConnect(std::shared_ptr<Context> context);
    std::shared_ptr<BlockData<bool>> data_;
    sptr<AAFwk::IAbilityConnection> callback_ = nullptr;
};
} // namespace OHOS::DataShare
#endif
