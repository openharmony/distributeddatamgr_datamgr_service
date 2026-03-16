/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define LOG_TAG "DataMiningSrcCli"
#include "runtime/source_client.h"

#include "log_print.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_ERROR = -1;

class RemoteSourceClient final : public SourceClient {
public:
    explicit RemoteSourceClient(EndpointConfig endpoint) : endpoint_(std::move(endpoint))
    {
    }

    int32_t OnInitialize() override
    {
        ZLOGE("remote source endpoint is not implemented, kind:%{public}d", static_cast<int32_t>(endpoint_.kind));
        return E_ERROR;
    }

    int32_t Trigger(std::shared_ptr<Context> context, std::shared_ptr<AsyncNotifier> notifier) override
    {
        (void)context;
        (void)notifier;
        ZLOGE("remote source trigger is not implemented");
        return E_ERROR;
    }

    int32_t Subscribe(
        const std::string &topic, std::shared_ptr<Context> context, std::shared_ptr<AsyncNotifier> notifier) override
    {
        (void)topic;
        (void)context;
        (void)notifier;
        ZLOGE("remote source subscribe is not implemented");
        return E_ERROR;
    }

    int32_t Unsubscribe(const std::string &topic, std::shared_ptr<Context> context) override
    {
        (void)topic;
        (void)context;
        return E_ERROR;
    }

    int32_t OnStop() override
    {
        return E_ERROR;
    }

private:
    EndpointConfig endpoint_;
};
} // namespace

std::shared_ptr<SourceClient> CreateSourceClient(const EndpointConfig &endpoint)
{
    // 当前先把 SA/extension source 的入口抽象出来，便于后续补专门的 IPC/extension bridge。
    // 现阶段真正跑通的是 lib source 路径。
    if (endpoint.kind == EndpointKind::SA || endpoint.kind == EndpointKind::EXTENSION) {
        return std::make_shared<RemoteSourceClient>(endpoint);
    }
    return nullptr;
}
} // namespace OHOS::DataMining
