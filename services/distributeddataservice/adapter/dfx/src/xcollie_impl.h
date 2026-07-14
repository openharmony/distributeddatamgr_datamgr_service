/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_XCOLLIE_IMPL_H
#define DISTRIBUTEDDATAMGR_XCOLLIE_IMPL_H

#include "dfx/xcollie.h"

namespace OHOS::DistributedData {
class XCollieImpl final : public XCollieDelegate {
public:
    static bool Init();
    int32_t SetTimer(const std::string &tag, uint32_t timeoutSeconds, std::function<void(void *)> func, void *arg,
                     uint32_t flag) override;
    void CancelTimer(int32_t id) override;

private:
    ~XCollieImpl() override = default;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_XCOLLIE_IMPL_H
