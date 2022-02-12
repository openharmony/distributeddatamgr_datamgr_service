/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef INTERCEPTED_DATA_IMPL_H
#define INTERCEPTED_DATA_IMPL_H

#include <cstddef>
#include <functional>
#include <vector>

#include "intercepted_data.h"
#include "macro_utils.h"
#include "single_ver_kv_entry.h"
#include "store_types.h"
#include "types_export.h"

namespace DistributedDB {
class InterceptedDataImpl : public InterceptedData {
public:
    InterceptedDataImpl(std::vector<SingleVerKvEntry *> dataItems, const std::function<int(const Value&)> &checkSchema);
    virtual ~InterceptedDataImpl();
    DISABLE_COPY_ASSIGN_MOVE(InterceptedDataImpl);

    std::vector<KVEntry> GetEntries() override;
    DBStatus ModifyKey(size_t index, const Key &newKey) override;
    DBStatus ModifyValue(size_t index, const Value &newValue) override;

    bool IsError() const;

private:
    bool CheckIndex(size_t index);
    void GetKvEntries();

    bool kvEntriesReady_;
    bool isError_;
    size_t totalLength_;
    size_t maxPacketSize_;
    std::function<int(const Value &)> checkSchema_;
    std::vector<SingleVerKvEntry *> dataItems_;
    std::vector<KVEntry> kvEntries_;
    std::vector<size_t> indexes_;
};
} // namespace DistributedDB
#endif // INTERCEPTED_DATA_IMPL_H