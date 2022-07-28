/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_KV_MANAGER_H
#define OHOS_KV_MANAGER_H
#include <mutex>
#include "distributed_kv_data_manager.h"
#include "kvstore_death_recipient.h"
#include "napi_queue.h"
#include "uv_queue.h"
#include "js_observer.h"

namespace OHOS::DistributedData {
struct ContextParam {
    std::string baseDir = "";
    std::string hapName = "";
    int32_t area = DistributedKv::Area::EL1;
};
class JsKVManager {
public:
    JsKVManager(const std::string &bundleName, napi_env env, ContextParam param);
    ~JsKVManager();

    static napi_value CreateKVManager(napi_env env, napi_callback_info info);

    static napi_value Constructor(napi_env env);

    static napi_value New(napi_env env, napi_callback_info info);

private:
    static napi_value GetKVStore(napi_env env, napi_callback_info info);
    static napi_value CloseKVStore(napi_env env, napi_callback_info info);
    static napi_value DeleteKVStore(napi_env env, napi_callback_info info);
    static napi_value GetAllKVStoreId(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);

private:
    class DeathRecipient : public DistributedKv::KvStoreDeathRecipient, public JSObserver {
    public:
        DeathRecipient(std::shared_ptr<UvQueue> uvQueue, napi_value callback) : JSObserver(uvQueue, callback) {};
        virtual ~DeathRecipient() = default;
        void OnRemoteDied() override;
    };

    DistributedKv::DistributedKvDataManager kvDataManager_ {};
    std::string bundleName_ {};
    std::mutex deathMutex_ {};
    std::list<std::shared_ptr<DeathRecipient>> deathRecipient_ {};
    std::shared_ptr<UvQueue> uvQueue_;
    std::shared_ptr<ContextParam> param_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_KV_MANAGER_H
