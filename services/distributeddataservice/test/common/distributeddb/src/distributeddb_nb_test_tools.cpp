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
#include "distributeddb_nb_test_tools.h"
#include <fstream>

#if defined(RUNNING_ON_LINUX)
#include <unistd.h>
#elif defined RUNNING_ON_WIN
#include <windows.h>
#endif

#ifndef USING_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif
using namespace std;
using namespace std::placeholders;
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

void DelegateMgrNbCallback::Callback(DBStatus status, KvStoreNbDelegate *kvStoreNbDelegate)
{
    this->status_ = status;
    this->kvStoreNbDelegate_ = kvStoreNbDelegate;
    MST_LOG("DelegateMgrNbCallback status:%d, kvStoreNbDelegate_null: %d", status, (kvStoreNbDelegate == nullptr));
}

DBStatus DelegateMgrNbCallback::GetStatus()
{
    return status_;
}

KvStoreNbDelegate *DelegateMgrNbCallback::GetKvStore()
{
    return kvStoreNbDelegate_;
}

KvStoreNbDelegate* DistributedDBNbTestTools::GetNbDelegateSuccess(KvStoreDelegateManager *&outManager,
    const DBParameters &param, const Option &optionParam, const string &dbPath)
{
    MST_LOG("GetNbDelegate isMemoryDb= %d, isEncryptedDb= %d", optionParam.isMemoryDb,
        optionParam.isEncryptedDb);
    SetDir(dbPath);
    if (param.storeId.empty() || param.appId.empty() || param.userId.empty()) {
        return nullptr;
    }

    // define a Callback to hold the KvStoreNbDelegate and status.
    DelegateMgrNbCallback delegateMgrCallback;
    function<void(DBStatus, KvStoreNbDelegate*)> function
        = bind(&DelegateMgrNbCallback::Callback, &delegateMgrCallback, _1, _2);

    // use appid and userid to initialize a kvStoreDelegateManager, and set the default cfg.
    if (outManager != nullptr) {
        delete outManager;
        outManager = nullptr;
    }
    KvStoreDelegateManager *manager = new (std::nothrow) KvStoreDelegateManager(param.appId, param.userId);
    if (manager == nullptr) {
        return nullptr;
    }
    DBStatus status = manager->SetKvStoreConfig({ .dataDir = dbPath});
    if (status != DBStatus::OK) {
        MST_LOG("%s SetConfig failed! Status= %d", TAG.c_str(), status);
        delete manager;
        manager = nullptr;
        return nullptr;
    }

    KvStoreNbDelegate::Option option = TransferNbOptionType(optionParam);
    // get kv store, then the Callback will save the status and delegate.
    manager->GetKvStore(param.storeId, option, function);
    status = delegateMgrCallback.GetStatus();
    if (status != DBStatus::OK) {
        MST_LOG("%s GetKvStore failed! Status= %d", TAG.c_str(), status);
        delete manager;
        manager = nullptr;
        return nullptr;
    }
    const KvStoreNbDelegate* delegate = const_cast<KvStoreNbDelegate *>(delegateMgrCallback.GetKvStore());
    if (delegate == nullptr) {
        MST_LOG("%s GetKvStore failed! delegate nullptr.", TAG.c_str());
        delete manager;
        manager = nullptr;
        return nullptr;
    }

    MST_LOG("%s GetKvStore success: %s %s %s %d", TAG.c_str(),
        param.storeId.c_str(), param.appId.c_str(), param.userId.c_str(), option.createIfNecessary);
    outManager = manager;
    return const_cast<KvStoreNbDelegate *>(delegate);
}

KvStoreNbDelegate* DistributedDBNbTestTools::GetNbDelegateStatus(KvStoreDelegateManager *&outManager,
    DBStatus &statusReturn, const DBParameters &param, const Option &optionParam)
{
    MST_LOG("GetNbDelegate isMemoryDb= %d, isEncryptedDb= %d", optionParam.isMemoryDb,
        optionParam.isEncryptedDb);
    SetDir(DistributedDBConstant::NB_DIRECTOR);
    if (param.storeId.empty() || param.appId.empty() || param.userId.empty()) {
        return nullptr;
    }

    // define a Callback to hold the KvStoreNbDelegate and status.
    DelegateMgrNbCallback delegateMgrCallback;
    function<void(DBStatus, KvStoreNbDelegate*)> function
        = bind(&DelegateMgrNbCallback::Callback, &delegateMgrCallback, _1, _2);

    // use appid and userid to initialize a kvStoreDelegateManager, and set the default cfg.
    if (outManager != nullptr) {
        delete outManager;
        outManager = nullptr;
    }
    KvStoreDelegateManager *manager1 = new (std::nothrow) KvStoreDelegateManager(param.appId, param.userId);
    if (manager1 == nullptr) {
        return nullptr;
    }
    statusReturn = manager1->SetKvStoreConfig({ .dataDir = DistributedDBConstant::NB_DIRECTOR });
    if (statusReturn != DBStatus::OK) {
        MST_LOG("%s SetConfig failed! Status= %d", TAG.c_str(), statusReturn);
        delete manager1;
        manager1 = nullptr;
        return nullptr;
    }

    KvStoreNbDelegate::Option option = TransferNbOptionType(optionParam);
    // get kv store, then the Callback will save the status and delegate.
    manager1->GetKvStore(param.storeId, option, function);
    statusReturn = delegateMgrCallback.GetStatus();
    if (statusReturn != DBStatus::OK) {
        MST_LOG("%s GetKvStore failed! Status= %d", TAG.c_str(), statusReturn);
        delete manager1;
        manager1 = nullptr;
        return nullptr;
    }
    const KvStoreNbDelegate* delegate = const_cast<KvStoreNbDelegate *>(delegateMgrCallback.GetKvStore());
    if (delegate == nullptr) {
        MST_LOG("%s GetKvStore failed! delegate nullptr.", TAG.c_str());
        delete manager1;
        manager1 = nullptr;
        return nullptr;
    }

    MST_LOG("%s GetKvStore success: %s %s %s %d", TAG.c_str(),
        param.storeId.c_str(), param.appId.c_str(), param.userId.c_str(), option.createIfNecessary);
    outManager = manager1;
    return const_cast<KvStoreNbDelegate *>(delegate);
}

DBStatus DistributedDBNbTestTools::GetNbDelegateStoresSuccess(KvStoreDelegateManager *&outManager,
    vector<KvStoreNbDelegate *> &outDelegateVec,
    const vector<string> &storeIds, const string &appId, const string &userId, const Option &optionParam)
{
    SetDir(DistributedDBConstant::NB_DIRECTOR);
    unsigned long opCnt;
    DBStatus status = DBStatus::OK;
    for (opCnt = 0; opCnt < storeIds.size(); ++opCnt) {
        if (storeIds[opCnt].empty() || appId.empty() || userId.empty()) {
            return INVALID_ARGS;
        }
    }

    // use appid and userid to initialize a kvStoreDelegateManager, and set the default cfg.
    KvStoreDelegateManager *manager = new (std::nothrow) KvStoreDelegateManager(appId, userId);
    if (manager == nullptr) {
        return DBStatus::DB_ERROR;
    }
    outDelegateVec.clear();
    KvStoreNbDelegate::Option option = TransferNbOptionType(optionParam);
    for (opCnt = 0; opCnt < storeIds.size(); ++opCnt) {
        // define a Callback to hold the KvStoreNbDelegate and status.
        DelegateMgrNbCallback delegateMgrCallback;
        function<void(DBStatus, KvStoreNbDelegate*)> function
            = bind(&DelegateMgrNbCallback::Callback, &delegateMgrCallback, _1, _2);

        status = manager->SetKvStoreConfig(DistributedDBConstant::CONFIG);
        if (status != DBStatus::OK) {
            MST_LOG("%s SetConfig failed! Status= %d", TAG.c_str(), status);
            goto END;
        }

        // get kv store, then the Callback will save the status and delegate.
        manager->GetKvStore(storeIds[opCnt], option, function);
        status = delegateMgrCallback.GetStatus();
        if (status != DBStatus::OK) {
            MST_LOG("%s GetKvStore failed! Status= %d", TAG.c_str(), status);
            goto END;
        }
        const KvStoreNbDelegate *delegate = const_cast<KvStoreNbDelegate *>(delegateMgrCallback.GetKvStore());
        if (delegate == nullptr) {
            MST_LOG("%s GetKvStore failed! delegate nullptr.", TAG.c_str());
            goto END;
        }
        outDelegateVec.push_back(const_cast<KvStoreNbDelegate *>(delegate));
    }
    outManager = manager;
    return OK;
END:
    delete manager;
    manager = nullptr;
    return status;
}

KvStoreNbDelegate::Option DistributedDBNbTestTools::TransferNbOptionType(const Option &optionParam)
{
    KvStoreNbDelegate::Option option;
    option.createIfNecessary = optionParam.createIfNecessary;
    option.isMemoryDb = optionParam.isMemoryDb;
    option.isEncryptedDb = optionParam.isEncryptedDb;
    option.cipher = optionParam.cipher;
#ifdef RELEASE_MODE_V2
    option.schema = optionParam.schema;
#endif // endif of RELEASE_MODE_V2
    (void)option.passwd.SetValue(optionParam.passwd.data(), optionParam.passwd.size());
#ifdef RELEASE_MODE_V3
    option.secOption.securityLabel = optionParam.secOption.securityLabel;
    option.secOption.securityFlag = optionParam.secOption.securityFlag;
    option.observer = optionParam.observer;
    option.key = optionParam.key;
    option.mode = optionParam.mode;
    option.conflictType = optionParam.conflictType;
    option.notifier = optionParam.notifier;
    option.conflictResolvePolicy = optionParam.conflictResolvePolicy;
    option.isNeedIntegrityCheck = optionParam.isNeedIntegrityCheck;
    option.isNeedRmCorruptedDb = optionParam.isNeedRmCorruptedDb;
    option.isNeedCompressOnSync = optionParam.isNeedCompressOnSync;
    option.compressionRate = optionParam.compressionRate;
#endif // end of RELEASE_MODE_V3
    return option;
}

// this static method is to compare if the two Value has the same data.
bool DistributedDBNbTestTools::isValueEquals(const DistributedDB::Value &v1, const DistributedDB::Value &v2)
{
    // just return false if the sizes are not the same.
    if (v1.size() != v2.size()) {
        return false;
    }

    // compare two Values char by char.
    return v1 == v2;
}

DistributedDB::DBStatus DistributedDBNbTestTools::Get(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const DistributedDB::Key &key, DistributedDB::Value &value)
{
    return kvStoreNbDelegate.Get(key, value);
}

DistributedDB::DBStatus DistributedDBNbTestTools::GetEntries(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const DistributedDB::Key &keyPrefix, std::vector<DistributedDB::Entry> &entries)
{
    return kvStoreNbDelegate.GetEntries(keyPrefix, entries);
}

DistributedDB::DBStatus DistributedDBNbTestTools::Put(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const DistributedDB::Key &key, const DistributedDB::Value &value, bool isNeedRetry, int waitTime)
{
    if (isNeedRetry) {
        DBStatus status = kvStoreNbDelegate.Put(key, value);
        if (status != OK && waitTime-- > 0) {
            MST_LOG("put records status: %d, and retry!", status);
            return DistributedDBNbTestTools::Put(kvStoreNbDelegate, key, value, isNeedRetry, waitTime);
        } else {
            return status;
        }
    }
    return kvStoreNbDelegate.Put(key, value);
}

DistributedDB::DBStatus DistributedDBNbTestTools::PutBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const std::vector<DistributedDB::Entry> &entries, bool isNeedRetry)
{
#ifdef RELEASE_MODE_V2
    DistributedDB::DBStatus status;
    for (int cnt = 0; cnt < static_cast<int>(entries.size()); cnt += 128) { // 128 is the max records of deleteBatch.
        auto last = std::min(static_cast<int>(entries.size()), cnt + 128); // 128 is the max records of deleteBatch.
        std::vector<DistributedDB::Entry> entriesBatch(entries.begin() + cnt, entries.begin() + last);

        int retryTimes = 1000;
        do {
            status = kvStoreNbDelegate.PutBatch(entriesBatch);
            if (status == OK) {
                break;
            } else if (status != BUSY && status != OK) {
                return status;
            }
        } while (isNeedRetry && retryTimes-- > 0);
    }
    return status;
#else
    MST_LOG("[DistributedDBNbTestTools::PutBatch] is NeedRetry is %d", isNeedRetry);
    DBStatus status;
    for (const auto &iter : entries) {
        status = kvStoreNbDelegate.Put(iter.key, iter.value);
        if (status != OK) {
            return status;
        }
    }
    return DBStatus::OK;
#endif // endif of RELEASE_MODE_V2
}

DistributedDB::DBStatus DistributedDBNbTestTools::Delete(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const DistributedDB::Key &key)
{
    return kvStoreNbDelegate.Delete(key);
}

DistributedDB::DBStatus DistributedDBNbTestTools::DeleteBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const std::vector<DistributedDB::Key> &keys, bool isNeedRetry)
{
#ifdef RELEASE_MODE_V2
    DistributedDB::DBStatus status;
    for (int cnt = 0; cnt < static_cast<int>(keys.size()); cnt = cnt + 128) { // 128 is the max records of deleteBatch.
        auto last = std::min(static_cast<int>(keys.size()), cnt + 128); // 128 is the max records of deleteBatch.
        std::vector<DistributedDB::Key> keysBatch(keys.begin() + cnt, keys.begin() + last);

        int retryTimes = 1000;
        do {
            status = kvStoreNbDelegate.DeleteBatch(keysBatch);
            if (status == OK) {
                break;
            } else if (status != BUSY && status != OK) {
                return status;
            }
        } while (isNeedRetry && retryTimes-- > 0);
    }
    return status;
#else
    MST_LOG("[DistributedDBNbTestTools::DeleteBatch] is NeedRetry is %d", isNeedRetry);
    DBStatus status;
    for (const auto &iter : keys) {
        status = kvStoreNbDelegate.Delete(iter);
        if (status != OK) {
            return status;
        }
    }
    return DBStatus::OK;
#endif // endif of RELEASE_MODE_V2
}

DistributedDB::DBStatus DistributedDBNbTestTools::GetLocal(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const DistributedDB::Key &key, DistributedDB::Value &value)
{
    return kvStoreNbDelegate.GetLocal(key, value);
}

DistributedDB::DBStatus DistributedDBNbTestTools::PutLocal(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const DistributedDB::Key &key, const DistributedDB::Value &value, bool isNeedRetry, int waitTime)
{
    if (isNeedRetry) {
        DBStatus status = kvStoreNbDelegate.PutLocal(key, value);
        if (status != OK && waitTime-- > 0) {
            MST_LOG("PutLocal records status: %d, and retry!", status);
            return DistributedDBNbTestTools::PutLocal(kvStoreNbDelegate, key, value, isNeedRetry, waitTime);
        } else {
            return status;
        }
    }
    return kvStoreNbDelegate.PutLocal(key, value);
}

DistributedDB::DBStatus DistributedDBNbTestTools::PutLocalBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const std::vector<DistributedDB::Entry> &entries)
{
#ifdef RELEASE_MODE_V3
    int cnt = 0;
    std::vector<DistributedDB::Entry> entriesBatch;
    DistributedDB::DBStatus status;
    for (const auto &iter : entries) {
        entriesBatch.push_back(iter);
        cnt++;
        if (cnt % BATCH_RECORDS == 0 || cnt == static_cast<int>(entries.size())) {
            status = kvStoreNbDelegate.PutLocalBatch(entriesBatch);
            if (status != DBStatus::OK) {
                return status;
            }
            entriesBatch.clear();
        }
    }
    return DBStatus::OK;
#else
    DistributedDB::DBStatus status;
    for (auto entry : entries) {
        status = kvStoreNbDelegate.PutLocal(entry.key, entry.value);
        if (status != DBStatus::OK) {
            return status;
        }
    }
    return DBStatus::OK;
#endif
}

DistributedDB::DBStatus DistributedDBNbTestTools::DeleteLocal(
    DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate, const DistributedDB::Key &key)
{
    return kvStoreNbDelegate.DeleteLocal(key);
}

DistributedDB::DBStatus DistributedDBNbTestTools::DeleteLocalBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const std::vector<DistributedDB::Key> &keys)
{
#ifdef RELEASE_MODE_V3
    int cnt = 0;
    std::vector<DistributedDB::Key> keysBatch;
    DistributedDB::DBStatus status;
    for (const auto &iter : keys) {
        keysBatch.push_back(iter);
        cnt++;
        if (cnt % BATCH_RECORDS == 0 || cnt == static_cast<int>(keys.size())) {
            status = kvStoreNbDelegate.DeleteLocalBatch(keysBatch);
            if (status != DBStatus::OK) {
                return status;
            }
            keysBatch.clear();
        }
    }
    return DBStatus::OK;
#else
    DistributedDB::DBStatus status;
    for (auto key : keys) {
        status = kvStoreNbDelegate.DeleteLocal(key);
        if (status != DBStatus::OK) {
            return status;
        }
    }
    return DBStatus::OK;
#endif
}

DistributedDB::DBStatus DistributedDBNbTestTools::RegisterObserver(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
    const DistributedDB::Key &key, unsigned int mode, DistributedDB::KvStoreObserver *observer)
{
    return kvStoreNbDelegate.RegisterObserver(key, mode, observer);
}

DistributedDB::DBStatus DistributedDBNbTestTools::UnRegisterObserver(
    DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate, const DistributedDB::KvStoreObserver *observer)
{
    return kvStoreNbDelegate.UnRegisterObserver(observer);
}

bool DistributedDBNbTestTools::CloseNbAndRelease(KvStoreDelegateManager *&manager, KvStoreNbDelegate *&delegate)
{
    bool result = true;
    if (delegate != nullptr && manager != nullptr) {
        result = (manager->CloseKvStore(delegate) == OK);
        delegate = nullptr;
        delete manager;
        manager = nullptr;
    } else {
        MST_LOG("Close Failed");
        return false;
    }
    return result;
}

bool EndCaseDeleteDB(DistributedDB::KvStoreDelegateManager *&manager,
    DistributedDB::KvStoreNbDelegate *&nbDelegate, const std::string &base, bool isMemoryDb)
{
    bool isResult = true;
    isResult = (manager->CloseKvStore(nbDelegate) == OK);
    MST_LOG("CloseKvStore result:%d", isResult);
    nbDelegate = nullptr;

    if (!isMemoryDb) {
        isResult = isResult && (manager->DeleteKvStore(base) == OK);
    }
    MST_LOG("DeleteKvStore result:%d", isResult);
    delete manager;
    manager = nullptr;
    return isResult;
}

void ConflictNbCallback::NotifyCallBack(const DistributedDB::KvStoreNbConflictData &data,
    std::vector<ConflictData> *&conflictData)
{
    MST_LOG("[ConflictCallback] Calling CallBack...");
    Key key;
    Value oldValue;
    Value newValue;
    data.GetKey(key);
    data.GetValue(KvStoreNbConflictData::ValueType::OLD_VALUE, oldValue);
    data.GetValue(KvStoreNbConflictData::ValueType::NEW_VALUE, newValue);
    conflictData->push_back({data.GetType(), key, oldValue, newValue,
        data.IsDeleted(KvStoreNbConflictData::ValueType::OLD_VALUE),
        data.IsDeleted(KvStoreNbConflictData::ValueType::NEW_VALUE),
        data.IsNative(KvStoreNbConflictData::ValueType::OLD_VALUE),
        data.IsNative(KvStoreNbConflictData::ValueType::NEW_VALUE)});
}

bool DistributedDBNbTestTools::ModifyDatabaseFile(const std::string &fileDir)
{
    MST_LOG("Modify file:%s", fileDir.c_str());
    std::fstream dataFile(fileDir, std::fstream::binary | std::fstream::out | std::fstream::in);
    if (!dataFile.is_open()) {
        MST_LOG("Open the database file failed");
        return false;
    }

    if (!dataFile.seekg(0, std::fstream::end)) {
        return false;
    }

    std::ios::pos_type pos = dataFile.tellg();
    if (pos < 0) {
        return false;
    } else {
        uint64_t fileSize = static_cast<uint64_t>(pos);
        if (fileSize < 1024) { // the least page size is 1024 bytes.
            MST_LOG("Invalid database file:%lld.", static_cast<long long>(fileSize));
            return false;
        }
    }

    uint32_t currentCount = 0x1F1F1F1F; // add the random value to corrupt the head.
    if (!dataFile.seekp(0)) {
        return false;
    }
    for (uint32_t i = 0; i < pos / sizeof(uint32_t); i++) {
        if (!dataFile.write(reinterpret_cast<char *>(&currentCount), sizeof(uint32_t))) {
            return false;
        }
    }
    dataFile.flush();
    dataFile.close();
    return true;
}
std::string DistributedDBNbTestTools::GetKvNbStoreDirectory(const DBParameters &param, const std::string &dbFilePath,
    const std::string &dbDir)
{
    std::string identifier = param.userId + "-" + param.appId + "-" + param.storeId;
    std::string identifierName = TransferStringToHashHexString(identifier);
    std::string filePath = dbDir + identifierName + "/" + dbFilePath;
    return filePath;
}
bool DistributedDBNbTestTools::MoveToNextFromBegin(KvStoreResultSet &resultSet,
    const vector<DistributedDB::Entry> &entries, int recordCnt)
{
    bool result = (static_cast<int>(entries.size()) >= recordCnt);
    if (!result) {
        MST_LOG("entries.size()) < recordCnt!!!");
        return result;
    }
    Entry entry;
    for (int position = -1; position < recordCnt; ++position) { // the first pos after getentries is -1.
        bool expectRes = resultSet.MoveToNext();
        if (position < (recordCnt - 1)) {
            result = result && expectRes;
        } else {
            result = result && (!expectRes);
        }
        if (!result) {
            MST_LOG("resultSet.MoveToNext() doesn't meet expectations!!!");
            break;
        }
        int positionGot = position + 1;
        result = result && (resultSet.GetPosition() == positionGot);
        if (!result) {
            MST_LOG("resultSet.GetPosition() != positionGot!!!");
            break;
        }
        if (position < (recordCnt - 1)) {
            result = result && (resultSet.GetEntry(entry) == OK);
            if (!result) {
                MST_LOG("resultSet.GetEntry() != OK");
                break;
            }
            result = result && (entry.key == entries[positionGot].key) && (entry.value == entries[positionGot].value);
            if (!result) {
                MST_LOG("entry != entries[positionGot]");
                break;
            }
        } else {
            result = result && (resultSet.GetEntry(entry) == NOT_FOUND);
        }
    }
    return result;
}

std::string DistributedDBNbTestTools::GetResourceDir()
{
    std::string dir;
    bool result = GetCurrentDir(dir);
    if (!result) {
        MST_LOG("[GetResourceDir] FAILED!");
        return "";
    }
#ifdef RUNNING_ON_SIMULATED_ENV
    dir = dir + "resource/";
#endif
    return dir;
}

bool DistributedDBNbTestTools::GetCurrentDir(std::string &dir)
{
    static const int maxFileLength = 1024;
    dir = "";
    char buffer[maxFileLength] = {0};
#if defined(RUNNING_ON_LINUX)
    int length = readlink("/proc/self/exe", buffer, maxFileLength);
#elif defined RUNNING_ON_WIN
    int length = -1;
    if (_getcwd(buffer, maxFileLength) != nullptr) {
        length = strlen(buffer);
    }
#endif
    if (length < 0 || length >= maxFileLength) {
        MST_LOG("read directory err length:%d", length);
        return false;
    }
    MST_LOG("DIR = %s", buffer);
    dir = buffer;
    if (dir.rfind("/") != std::string::npos) {
        dir.erase(dir.rfind("/") + 1);
    }
    if ((access(dir.c_str(), F_OK)) != E_OK) {
        return false;
    }
    return true;
}
bool DistributedDBNbTestTools::CheckNbNoRecord(KvStoreNbDelegate *&delegate, const Key &key, bool bIsLocalQuery)
{
    Value realValue;
    bool result = true;
    DBStatus status;
    if (!bIsLocalQuery) {
        status = delegate->Get(key, realValue);
    } else {
        status = delegate->GetLocal(key, realValue);
    }
    result = result && (status == NOT_FOUND);
    if (!result) {
        MST_LOG("[DistributedDBNbTestTools] bIsLocalQuery:%d, status: %d, realValue.size() is: %zd",
            bIsLocalQuery, status, realValue.size());
        return result;
    }
    return (realValue.size() == 0);
}

bool DistributedDBNbTestTools::CheckNbRecord(KvStoreNbDelegate *&delegate,
    const Key &key, const Value &value, bool bIsLocalQuery)
{
    Value realValue;
    bool result = true;
    if (!bIsLocalQuery) {
        if (value.empty()) {
            return (delegate->Get(key, realValue) == NOT_FOUND);
        } else {
            result = result && (delegate->Get(key, realValue) == OK);
        }
    } else {
        if (value.empty()) {
            return (delegate->GetLocal(key, realValue) == NOT_FOUND);
        } else {
            result = result && (delegate->GetLocal(key, realValue) == OK);
        }
    }

    if (!result) {
        MST_LOG("[DistributedDBNbTestTools] Get Record failed");
        return result;
    }
    result = result && (realValue == value);
    if (!result) {
        string realString(realValue.begin(), realValue.end());
        MST_LOG("[DistributedDBNbTestTools] check the value(LocalQuery:%d) failed, and the realGetValue is %s",
            bIsLocalQuery, realString.c_str());
        return result;
    }
    return result;
}
void KvStoreNbCorruptInfo::CorruptNewCallBack(const std::string &appId, const std::string &userId,
    const std::string &storeId, DistributedDB::KvStoreDelegateManager *&manager, CallBackParam &pathResult)
{
    MST_LOG("Begin to recover the corrupt DB.");
    manager->DeleteKvStore(storeId);
    delete manager;
    manager = nullptr;
    DBParameters param(storeId, appId, userId);
    KvStoreNbDelegate *delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(manager, param, g_option);
    if (manager == nullptr || delegate == nullptr) {
        MST_LOG("[KvStoreNbCorruptInfo::CorruptNewCallBack] Get delegate or manager failed!");
        pathResult.result = false;
        return;
    } else {
        if (delegate->Import(pathResult.path, NULL_PASSWD) != OK) {
            MST_LOG("[KvStoreNbCorruptInfo::CorruptNewCallBack] Import failed!");
            pathResult.result = false;
        }
        if (manager->CloseKvStore(delegate) != OK) {
            MST_LOG("[KvStoreNbCorruptInfo::CorruptNewCallBack] CloseKvStore(delegate) failed!");
            pathResult.result = false;
        }
    }
    delegate = nullptr;
    delete manager;
    manager = nullptr;
}

void KvStoreNbCorruptInfo::CorruptCallBackOfImport(const std::string &appId, const std::string &userId,
    const std::string &storeId, DistributedDB::KvStoreNbDelegate *&delegate, CallBackParam &pathResult)
{
    MST_LOG("Begin to Import the recover db files.");
    MST_LOG("The corrupt Db is %s, %s, %s", appId.c_str(), userId.c_str(), storeId.c_str());
    CipherPassword password;
    (void)password.SetValue(FILE_PASSWD_VECTOR_1.data(), FILE_PASSWD_VECTOR_1.size());
    if (delegate->Import(pathResult.path, password) != OK) {
        MST_LOG("[KvStoreNbCorruptInfo::CorruptCallBackOfImport] Import failed!");
        pathResult.result = false;
    }
}

void KvStoreNbCorruptInfo::CorruptCallBackOfExport(const std::string &appId, const std::string &userId,
    const std::string &storeId, DistributedDB::KvStoreNbDelegate *&delegate, CallBackParam &pathResult)
{
    MST_LOG("The corrupt Db is %s, %s, %s", appId.c_str(), userId.c_str(), storeId.c_str());
    MST_LOG("Begin to Export the DB data.");
    if (delegate->Export(pathResult.path, NULL_PASSWD) != INVALID_PASSWD_OR_CORRUPTED_DB) {
        MST_LOG("[KvStoreNbCorruptInfo::CorruptCallBackOfExport] Export failed!");
        pathResult.result = false;
    }
}
