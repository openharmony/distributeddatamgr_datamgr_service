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

#include "kvstorenbdelegate_curd_fuzzer.h"
#include <list>
#include "distributeddb_tools_test.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_observer.h"
#include "platform_specific.h"

class KvStoreNbDelegateCURDFuzzer {
    /* Keep C++ file names the same as the class name. */
};

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;

class KvStoreObserverFuzzTest : public DistributedDB::KvStoreObserver {
public:
    KvStoreObserverFuzzTest();
    ~KvStoreObserverFuzzTest() = default;
    KvStoreObserverFuzzTest(const KvStoreObserverFuzzTest &) = delete;
    KvStoreObserverFuzzTest& operator=(const KvStoreObserverFuzzTest &) = delete;
    KvStoreObserverFuzzTest(KvStoreObserverFuzzTest &&) = delete;
    KvStoreObserverFuzzTest& operator=(KvStoreObserverFuzzTest &&) = delete;
    // callback function will be called when the db data is changed.
    void OnChange(const DistributedDB::KvStoreChangedData &);

    // reset the callCount_ to zero.
    void ResetToZero();
    // get callback results.
    unsigned long GetCallCount() const;
    const std::list<DistributedDB::Entry> &GetEntriesInserted() const;
    const std::list<DistributedDB::Entry> &GetEntriesUpdated() const;
    const std::list<DistributedDB::Entry> &GetEntriesDeleted() const;
    bool IsCleared() const;

private:
    unsigned long callCount_ = 0;
    bool isCleared_ = false;
    std::list<DistributedDB::Entry> inserted_ {};
    std::list<DistributedDB::Entry> updated_ {};
    std::list<DistributedDB::Entry> deleted_ {};
};

KvStoreObserverFuzzTest::KvStoreObserverFuzzTest()
{
    callCount_ = 0;
}

void KvStoreObserverFuzzTest::OnChange(const KvStoreChangedData &data)
{
    callCount_++;
    inserted_ = data.GetEntriesInserted();
    updated_ = data.GetEntriesUpdated();
    deleted_ = data.GetEntriesDeleted();
    isCleared_ = data.IsCleared();
}

void KvStoreObserverFuzzTest::ResetToZero()
{
    callCount_ = 0;
    isCleared_ = false;
    inserted_.clear();
    updated_.clear();
    deleted_.clear();
}

unsigned long KvStoreObserverFuzzTest::GetCallCount() const
{
    return callCount_;
}

const std::list<Entry> &KvStoreObserverFuzzTest::GetEntriesInserted() const
{
    return inserted_;
}

const std::list<Entry> &KvStoreObserverFuzzTest::GetEntriesUpdated() const
{
    return updated_;
}
const std::list<Entry> &KvStoreObserverFuzzTest::GetEntriesDeleted() const
{
    return deleted_;
}

bool KvStoreObserverFuzzTest::IsCleared() const
{
    return isCleared_;
}

std::vector<Entry> CreateEntries(const uint8_t* data, size_t size, std::vector<Key> keys)
{
    std::vector<Entry> entries;
    // key'length is less than 1024.
    auto count = static_cast<int>(std::min(size, size_t(1024)));
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = std::vector<uint8_t>(data, data + i);
        keys.push_back(entry.key);
        entry.value = std::vector<uint8_t>(data, data + size);
        entries.push_back(entry);
    }
    return entries;
}

void FuzzCURD(const uint8_t* data, size_t size, KvStoreNbDelegate *kvNbDelegatePtr)
{
    auto observer = new (std::nothrow) KvStoreObserverFuzzTest;
    if ((observer == nullptr) || (kvNbDelegatePtr == nullptr)) {
        return;
    }
    Key key = std::vector<uint8_t>(data, data + (size % 1024)); /* 1024 is max */
    Value value = std::vector<uint8_t>(data, data + size);
    kvNbDelegatePtr->RegisterObserver(key, size, observer);
    kvNbDelegatePtr->SetConflictNotifier(size, [](const KvStoreNbConflictData &data) {
        (void)data.GetType();
    });

    Value valueRead;
    kvNbDelegatePtr->PutLocal(key, value);
    kvNbDelegatePtr->GetLocal(key, valueRead);
    kvNbDelegatePtr->DeleteLocal(key);
    kvNbDelegatePtr->Put(key, value);
    kvNbDelegatePtr->Put(key, value);
    std::vector<Entry> vect;
    kvNbDelegatePtr->GetEntries(key, vect);
    kvNbDelegatePtr->Delete(key);
    kvNbDelegatePtr->Get(key, valueRead);
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(data, size, keys);
    kvNbDelegatePtr->PutBatch(tmp);
    if (!keys.empty()) {
        /* random deletePublic updateTimestamp 2 */
        kvNbDelegatePtr->UnpublishToLocal(keys[0], (data[0] > data[1]), (data[2] > data[1]));
    }
    kvNbDelegatePtr->DeleteBatch(keys);
    kvNbDelegatePtr->UnRegisterObserver(observer);
    kvNbDelegatePtr->PutLocalBatch(tmp);

    if (!keys.empty()) {
        /* random deletePublic updateTimestamp 2 */
        kvNbDelegatePtr->PublishLocal(keys[0], (data[0] > data[1]), (data[2] > data[1]), nullptr);
    }
    kvNbDelegatePtr->DeleteBatch(keys);
    std::string rawString(reinterpret_cast<const char *>(data), size);
    kvNbDelegatePtr->RemoveDeviceData(rawString);
}

void CombineTest(const uint8_t* data, size_t size, KvStoreNbDelegate::Option &option)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    kvManager.SetKvStoreConfig(config);
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    kvManager.GetKvStore("distributed_nb_delegate_test", option,
        [&kvNbDelegatePtr] (DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    FuzzCURD(data, size, kvNbDelegatePtr);
    kvManager.CloseKvStore(kvNbDelegatePtr);
    kvManager.DeleteKvStore("distributed_nb_delegate_test");
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    DistributedDB::KvStoreNbDelegate::Option option = {true, false, false};
    OHOS::CombineTest(data, size, option);
    option = {true, true, false};
    OHOS::CombineTest(data, size, option);
    return 0;
}

