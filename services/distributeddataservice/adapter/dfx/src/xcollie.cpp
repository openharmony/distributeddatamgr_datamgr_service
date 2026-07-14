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

#include "xcollie_impl.h"

#include "xcollie/xcollie.h"

namespace OHOS::DistributedData {
__attribute__((used)) static bool g_isInit = XCollieImpl::Init();

bool XCollieImpl::Init()
{
    static XCollieImpl xcollieImpl;
    (void)XCollieDelegate::RegisterXCollieInstance(&xcollieImpl);
    return true;
}

int32_t XCollieImpl::SetTimer(const std::string &tag, uint32_t timeoutSeconds, std::function<void(void *)> func,
                              void *arg, uint32_t flag)
{
    uint32_t xCollieFlag = 0;
    if ((flag & XCollie::XCOLLIE_LOG) == XCollie::XCOLLIE_LOG) {
        xCollieFlag |= HiviewDFX::XCOLLIE_FLAG_LOG;
    }
    if ((flag & XCollie::XCOLLIE_RECOVERY) == XCollie::XCOLLIE_RECOVERY) {
        xCollieFlag |= HiviewDFX::XCOLLIE_FLAG_RECOVERY;
    }
    return HiviewDFX::XCollie::GetInstance().SetTimer(tag, timeoutSeconds, func, arg, xCollieFlag);
}

void XCollieImpl::CancelTimer(int32_t id)
{
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
}
} // namespace OHOS::DistributedData
