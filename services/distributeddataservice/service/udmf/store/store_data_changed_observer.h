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

#ifndef UDMF_STORE_DATA_CHANGED_OBSERVER_H
#define UDMF_STORE_DATA_CHANGED_OBSERVER_H

#include "kv_store_observer.h"
namespace OHOS {
namespace UDMF {
class AcceptableInfoObserver : public DistributedDB::KvStoreObserver {
public:
    AcceptableInfoObserver() = default;
    ~AcceptableInfoObserver() override = default;

    void OnChange(const DistributedDB::KvStoreChangedData &data) override;
};

class RuntimeObserver : public DistributedDB::KvStoreObserver {
public:
    AcceptableInfoObserver() = default;
    ~AcceptableInfoObserver() override = default;

    void OnChange(const DistributedDB::KvStoreChangedData &data) override;
};

class ObserverFac {
public:
    static std::shared_ptr<DistributedDB::KvStoreObserver> GetObserver(uint32_t type)
    {
        if (type == ACCEPTABLE_INFO) {
            return std::make_shared<AcceptableInfoObserver>();
        } else if (type == RUNTIME) {
            return std::make_shared<RuntimeObserver>();
        } else {
            return nullptr;
        }
    }

    enum ObserverType : uint32_t {
        ACCEPTABLE_INFO = 0,
        RUNTIME,
    };
};
} // namespace UDMF
} // namespace OHOS

#endif // UDMF_STORE_DATA_CHANGED_OBSERVER_H