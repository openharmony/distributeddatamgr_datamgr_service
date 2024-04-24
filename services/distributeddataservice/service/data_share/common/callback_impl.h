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
#ifndef DATASHARESERVICE_CALLBACK_IMPL_H
#define DATASHARESERVICE_CALLBACK_IMPL_H

#include "ability_connect_callback_stub.h"
#include "block_data.h"

namespace OHOS::DataShare {
class CallbackImpl : public AAFwk::AbilityConnectionStub {
public:
    explicit CallbackImpl(std::shared_ptr<BlockData<bool>> data) : data_(data) {}
    void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override
    {
        if (data_ == nullptr) {
            return;
        }
        data_->SetValue(true);
    }
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override
    {
        if (data_ == nullptr) {
            return;
        }
        data_->SetValue(false);
    }

private:
    std::shared_ptr<BlockData<bool>> data_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_CALLBACK_IMPL_H
