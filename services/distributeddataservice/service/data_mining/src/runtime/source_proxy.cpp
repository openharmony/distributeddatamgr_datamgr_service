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

#include "runtime/source_proxy.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_OK = 0;
constexpr int32_t E_ERROR = -1;
}

SourceProxy::SourceProxy(std::string name, EndpointConfig endpoint, std::shared_ptr<PluginLoader> loader,
    std::shared_ptr<SourceClient> client, std::shared_ptr<Source> source)
    : name_(std::move(name)),
      endpoint_(std::move(endpoint)),
      loader_(std::move(loader)),
      client_(std::move(client)),
      source_(std::move(source))
{
}

std::string SourceProxy::GetName() const
{
    return name_;
}

int32_t SourceProxy::OnInitialize()
{
    std::lock_guard<std::mutex> lock(mutex_);
    // source 有三种来源：
    // 1. 已注入的测试/内存对象
    // 2. 本地动态库，首次使用时延迟加载
    // 3. SA/extension 远端 source，通过 SourceClient 转发
    if (source_ != nullptr) {
        return source_->OnInitialize();
    }
    if (endpoint_.kind == EndpointKind::LIBRARY) {
        int32_t status = EnsureSourceLocked();
        if (status != E_OK || source_ == nullptr) {
            return E_ERROR;
        }
        return source_->OnInitialize();
    }
    if (client_ != nullptr) {
        return client_->OnInitialize();
    }
    return E_ERROR;
}

int32_t SourceProxy::Trigger(std::shared_ptr<Context> context, std::shared_ptr<AsyncNotifier> notifier)
{
    if (notifier == nullptr) {
        return E_ERROR;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    // 触发型 source 的实际执行入口。DDMS timer/manual trigger 最终都会落到这里。
    if (source_ != nullptr) {
        return source_->Trigger(context, notifier);
    }
    if (endpoint_.kind == EndpointKind::LIBRARY) {
        int32_t status = EnsureSourceLocked();
        if (status != E_OK || source_ == nullptr) {
            return E_ERROR;
        }
        return source_->Trigger(context, notifier);
    }
    if (client_ != nullptr) {
        return client_->Trigger(context, notifier);
    }
    return E_ERROR;
}

int32_t SourceProxy::Subscribe(
    const std::string &topic, std::shared_ptr<Context> context, std::shared_ptr<AsyncNotifier> notifier)
{
    if (notifier == nullptr) {
        return E_ERROR;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    // 订阅型 source 会在 DDMS 侧建立数据捐赠/事件订阅，回调再通过 notifier 进入 ETL SA。
    if (source_ != nullptr) {
        return source_->Subscribe(topic, context, notifier);
    }
    if (endpoint_.kind == EndpointKind::LIBRARY) {
        int32_t status = EnsureSourceLocked();
        if (status != E_OK || source_ == nullptr) {
            return E_ERROR;
        }
        return source_->Subscribe(topic, context, notifier);
    }
    if (client_ != nullptr) {
        return client_->Subscribe(topic, context, notifier);
    }
    return E_ERROR;
}

int32_t SourceProxy::Unsubscribe(const std::string &topic, std::shared_ptr<Context> context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (source_ != nullptr) {
        return source_->Unsubscribe(topic, context);
    }
    if (client_ != nullptr) {
        return client_->Unsubscribe(topic, context);
    }
    return E_ERROR;
}

int32_t SourceProxy::OnStop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (source_ != nullptr) {
        return source_->OnStop();
    }
    if (client_ != nullptr) {
        return client_->OnStop();
    }
    return E_ERROR;
}

int32_t SourceProxy::EnsureSourceLocked()
{
    if (source_ != nullptr) {
        return E_OK;
    }
    // 本地 lib 场景只在真正需要执行 source 时做 dlopen，降低 DDMS 常驻内存。
    if (loader_ == nullptr || endpoint_.libraryPath.empty()) {
        return E_ERROR;
    }
    auto module = loader_->Load(endpoint_.libraryPath);
    if (module == nullptr) {
        return E_ERROR;
    }
    source_ = module->CreateSource(name_);
    return source_ == nullptr ? E_ERROR : E_OK;
}
} // namespace OHOS::DataMining
