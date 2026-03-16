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

#include "runtime/sink_proxy.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_OK = 0;
constexpr int32_t E_ERROR = -1;
}

SinkProxy::SinkProxy(std::string name, EndpointConfig endpoint, std::shared_ptr<PluginLoader> loader,
    std::shared_ptr<Sink> sink)
    : name_(std::move(name)), endpoint_(std::move(endpoint)), loader_(std::move(loader)), sink_(std::move(sink))
{
}

int32_t SinkProxy::Save(std::shared_ptr<Context> context, std::shared_ptr<DataValue> data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (sink_ != nullptr) {
        return sink_->Save(context, data);
    }
    if (endpoint_.kind != EndpointKind::LIBRARY) {
        return E_ERROR;
    }
    int32_t status = EnsureSinkLocked();
    if (status != E_OK || sink_ == nullptr) {
        return E_ERROR;
    }
    return sink_->Save(context, data);
}

int32_t SinkProxy::EnsureSinkLocked()
{
    if (sink_ != nullptr) {
        return E_OK;
    }
    if (loader_ == nullptr || endpoint_.libraryPath.empty()) {
        return E_ERROR;
    }
    auto module = loader_->Load(endpoint_.libraryPath);
    if (module == nullptr) {
        return E_ERROR;
    }
    sink_ = module->CreateSink(name_);
    return sink_ == nullptr ? E_ERROR : E_OK;
}
} // namespace OHOS::DataMining
