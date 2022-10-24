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
#include "distributed_kv_data_manager_fuzzer.h"

#include <vector>
#include <sys/stat.h>

#include "distributed_kv_data_manager.h"
#include "kvstore_death_recipient.h"
#include "types.h"

using namespace OHOS;
using namespace OHOS::DistributedKv;


namespace OHOS {
static std::shared_ptr<SingleKvStore> singleKvStore_ = nullptr;

DistributedKvDataManager manager;
Options create;
Options noCreate;

UserId userId;

AppId appId;
StoreId storeId64;
StoreId storeId65;
StoreId storeIdTest;
StoreId storeIdEmpty;

Entry entryA;
Entry entryB;

class MyDeathRecipient : public KvStoreDeathRecipient {
public:
    MyDeathRecipient()
    {
    }
    virtual ~MyDeathRecipient()
    {
    }
    void OnRemoteDied() override
    {
    }
};

class DeviceObserverTestImpl : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_;
    std::vector<Entry> updateEntries_;
    std::vector<Entry> deleteEntries_;
    bool isClear_ = false;
    DeviceObserverTestImpl();
    ~DeviceObserverTestImpl()
    {
    }

    DeviceObserverTestImpl(const DeviceObserverTestImpl &) = delete;
    DeviceObserverTestImpl &operator=(const DeviceObserverTestImpl &) = delete;
    DeviceObserverTestImpl(DeviceObserverTestImpl &&) = delete;
    DeviceObserverTestImpl &operator=(DeviceObserverTestImpl &&) = delete;

    void OnChange(const ChangeNotification &changeNotification);
};

void DeviceObserverTestImpl::OnChange(const ChangeNotification &changeNotification)
{
    const auto &insert = changeNotification.GetInsertEntries();
    insertEntries_.clear();
    for (const auto &entry : insert) {
        insertEntries_.push_back(entry);
    }

    const auto &update = changeNotification.GetUpdateEntries();
    updateEntries_.clear();
    for (const auto &entry : update) {
        updateEntries_.push_back(entry);
    }

    const auto &del = changeNotification.GetDeleteEntries();
    deleteEntries_.clear();
    for (const auto &entry : del) {
        deleteEntries_.push_back(entry);
    }

    isClear_ = changeNotification.IsClear();
}

DeviceObserverTestImpl::DeviceObserverTestImpl()
{
    insertEntries_ = {};
    updateEntries_ = {};
    deleteEntries_ = {};
    isClear_ = false;
}

class DeviceSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    void SyncCompleted(const std::map<std::string, Status> &results);
};

void DeviceSyncCallbackTestImpl::SyncCompleted(const std::map<std::string, Status> &results)
{
}

void SetUpTestCase(void)
{
    DistributedKvDataManager manager;

    StoreId storeId = { "kvdatamanager_test" };

    userId.userId = "account0";
    appId.appId = "odmf";
    create.createIfMissing = true;
    create.encrypt = false;
    create.autoSync = true;
    create.kvStoreType = SINGLE_VERSION;
    create.area = EL1;
    create.baseDir = std::string("/data/service/el1/public/database/") + appId.appId;
    mkdir(create.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    manager.CloseAllKvStore(appId);
    manager.DeleteAllKvStore(appId, create.baseDir);
}

void TearDown(void)
{
    manager.CloseAllKvStore(appId);
    manager.DeleteAllKvStore(appId, create.baseDir);
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void GetKvStoreFuzz(const uint8_t *data, size_t size)
{
    StoreId storeId;
    storeId.storeId = std::string(data, data + size);
    std::shared_ptr<SingleKvStore> notExistKvStore;
    manager.GetSingleKvStore(create, appId, storeId, notExistKvStore);
    std::shared_ptr<SingleKvStore> existKvStore;
    manager.GetSingleKvStore(noCreate, appId, storeId, existKvStore);
    manager.CloseKvStore(appId, storeId);
    manager.DeleteKvStore(appId, storeId);
}

void GetAllKvStoreFuzz(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    manager.GetAllKvStoreId(appId, storeIds);

    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeId_base(data, data + size);
    int sum = 10;
    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeId_base + "_" + std::to_string(i);
        manager.GetSingleKvStore(create, appId, storeId, KvStore);
    }
    manager.GetAllKvStoreId(appId, storeIds);
    manager.CloseAllKvStore(appId);

    manager.GetAllKvStoreId(appId, storeIds);
}

void CloseKvStoreFuzz(const uint8_t *data, size_t size)
{
    StoreId storeId;
    storeId.storeId = std::string(data, data + size);
    manager.CloseKvStore(appId, storeId);
    std::shared_ptr<SingleKvStore> kvStore;
    manager.GetSingleKvStore(create, appId, storeId, kvStore);
    manager.CloseKvStore(appId, storeId);
    manager.CloseKvStore(appId, storeId);
}

void DeleteKvStoreFuzz(const uint8_t *data, size_t size)
{
    StoreId storeId;
    storeId.storeId = std::string(data, data + size);
    manager.DeleteKvStore(appId, storeId, create.baseDir);

    std::shared_ptr<SingleKvStore> kvStore;
    manager.GetSingleKvStore(create, appId, storeId, kvStore);
    manager.CloseKvStore(appId, storeId);
    manager.DeleteKvStore(appId, storeId, create.baseDir);

    manager.CloseKvStore(appId, storeId);
}

void DeleteAllKvStoreFuzz1(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    manager.GetAllKvStoreId(appId, storeIds);

    manager.DeleteAllKvStore(appId, create.baseDir);
    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeId_base(data, data + size);
    int sum = 10;
    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeId_base + "_" + std::to_string(i);
        manager.GetSingleKvStore(create, appId, storeId, KvStore);

        manager.CloseKvStore(appId, storeId);
    }
    manager.DeleteAllKvStore(appId, create.baseDir);
}

void DeleteAllKvStoreFuzz2(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    manager.GetAllKvStoreId(appId, storeIds);

    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeId_base(data, data + size);
    manager.GetSingleKvStore(create, appId, storeIdTest, KvStore);
    manager.CloseKvStore(appId, storeIdTest);
    int sum = 10;
    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeId_base + "_" + std::to_string(i);
        manager.GetSingleKvStore(create, appId, storeId, KvStore);
    }
    manager.DeleteAllKvStore(appId, create.baseDir);
}

void DeleteAllKvStoreFuzz3(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    manager.GetAllKvStoreId(appId, storeIds);

    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeId_base(data, data + size);
    int sum = 10;
    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeId_base + "_" + std::to_string(i);
        manager.GetSingleKvStore(create, appId, storeId, KvStore);
    }
    manager.DeleteAllKvStore(appId, create.baseDir);
}

void RegisterKvStoreServiceDeathRecipientFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient = std::make_shared<MyDeathRecipient>();
    manager.RegisterKvStoreServiceDeathRecipient(kvStoreDeathRecipient);
    kvStoreDeathRecipient->OnRemoteDied();
}

void UnRegisterKvStoreServiceDeathRecipientFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient = std::make_shared<MyDeathRecipient>();
    manager.RegisterKvStoreServiceDeathRecipient(kvStoreDeathRecipient);
    kvStoreDeathRecipient->OnRemoteDied();
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::SetUpTestCase();
    OHOS::GetKvStoreFuzz(data, size);
    OHOS::GetAllKvStoreFuzz(data, size);
    OHOS::CloseKvStoreFuzz(data, size);
    OHOS::DeleteKvStoreFuzz(data, size);
    OHOS::DeleteAllKvStoreFuzz1(data, size);
    OHOS::DeleteAllKvStoreFuzz2(data, size);
    OHOS::DeleteAllKvStoreFuzz3(data, size);
    OHOS::RegisterKvStoreServiceDeathRecipientFuzz(data, size);
    OHOS::UnRegisterKvStoreServiceDeathRecipientFuzz(data, size);
    OHOS::TearDown();
    /* Run your code on data */
    return 0;
}