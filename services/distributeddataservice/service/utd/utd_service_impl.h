/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef UTD_SERVICE_IMPL_H
#define UTD_SERVICE_IMPL_H

#include "utd_notifier_proxy.h"
#include "utd_service_stub.h"
namespace OHOS {
namespace UDMF {
/*
 * UTD server implementation
 */
class UtdServiceImpl final : public UtdServiceStub {
public:
    UtdServiceImpl() = default;
    ~UtdServiceImpl() = default;
    int32_t OnBind(const BindInfo &bindInfo) override;
    int32_t RegServiceNotifier(sptr<IRemoteObject> iUtdNotifier) override;
    int32_t RegisterTypeDescriptors(const std::vector<TypeDescriptorCfg> &descriptors) override;
    int32_t UnregisterTypeDescriptors(const std::vector<std::string> &typeIds) override;

private:
    int32_t NotifyUtdClients(uint32_t callingTokenId);
    bool GetHapBundleNameByToken(uint32_t tokenId, std::string &bundleName);
    bool VerifyPermission(const std::string &permission, uint32_t callerTokenId);

    class Factory {
    public:
        Factory();
        ~Factory();
    private:
        std::shared_ptr<UtdServiceImpl> product_;
    };
    static Factory factory_;

    std::shared_ptr<ExecutorPool> executors_;
    ConcurrentMap<uint32_t, sptr<UtdNotifierProxy>> utdNotifiers_;
    std::atomic<uint32_t> foundationTokenId_ = 0;
};
} // namespace UDMF
} // namespace OHOS
#endif // UTD_SERVICE_IMPL_H
