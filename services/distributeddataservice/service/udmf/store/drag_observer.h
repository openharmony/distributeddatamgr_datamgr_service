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

#ifndef UDMF_DRAG_OBSERVER_H
#define UDMF_DRAG_OBSERVER_H

#include "kv_store_observer.h"

#include "types_export.h"
namespace OHOS {
namespace UDMF {

class DragObserver : public DistributedDB::KvStoreObserver {
public:
    DragObserver() = default;
    ~DragObserver() override = default;

    void OnChange(const DistributedDB::KvStoreChangedData &data) override;
private:
    void ExtractEntries(const DistributedDB::KvStoreChangedData &data,
        std::vector<DistributedDB::Entry> &runtimeEntries,
        std::vector<DistributedDB::Entry> &deleteRuntimeEntries);
    void ProcessRuntimeInfo(const std::vector<DistributedDB::Entry> &runtimeEntries);
    void ProcessDelete(const std::vector<DistributedDB::Entry> &deleteEntries);
    bool IsRuntimeKey(const std::vector<uint8_t> &key);
    void CollectIfRuntimeKey(const std::list<DistributedDB::Entry> &srcEntries,
        std::vector<DistributedDB::Entry> &dstEntries);
};

} // namespace UDMF
} // namespace OHOS

#endif // UDMF_DRAG_OBSERVER_H