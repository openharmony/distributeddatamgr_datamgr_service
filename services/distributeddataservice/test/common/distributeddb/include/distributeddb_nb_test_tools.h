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
#ifndef DISTRIBUTEDDB_NB_TEST_TOOLS_H
#define DISTRIBUTEDDB_NB_TEST_TOOLS_H

#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_observer.h"
#ifdef RELEASE_MODE_V2
#include "query.h"
#endif
#include "distributeddb_data_generator.h"
#include "distributed_test_tools.h"

struct DBParameters {
    std::string storeId;
    std::string appId;
    std::string userId;
    DBParameters(std::string storeIdStr, std::string appIdStr, std::string userIdStr)
        : storeId(storeIdStr), appId(appIdStr), userId(userIdStr)
    {
    }
};

struct Option {
    bool createIfNecessary = true;
    bool isMemoryDb = false; // true represents using a memory database
    bool isEncryptedDb = false; // whether need encrypt
    DistributedDB::CipherType cipher = DistributedDB::CipherType::DEFAULT; // cipher type
    std::vector<uint8_t> passwd; // cipher password
#ifdef RELEASE_MODE_V2
    std::string schema;
    bool createDirByStoreIdOnly = false;
#endif // endif of RELEASE_MODE_V2
#ifdef RELEASE_MODE_V3
    DistributedDB::SecurityOption secOption;
    DistributedDB::KvStoreObserver *observer = nullptr;
    DistributedDB::Key key;
    unsigned int mode = 0;
    int conflictType = 0;
    DistributedDB::KvStoreNbConflictNotifier notifier = nullptr;
    int conflictResolvePolicy = DistributedDB::LAST_WIN;
    bool isNeedIntegrityCheck = false;
    bool isNeedRmCorruptedDb = false;
    bool isNeedCompressOnSync = false;
    uint8_t compressionRate = 100; // default compression rate 100%, that means not compressing
#endif // end of RELEASE_MODE_V3
    Option(bool createIfNecessary, bool isMemoryDb, bool isEncryptedDb, DistributedDB::CipherType cipher,
        std::vector<uint8_t> passwd)
        : createIfNecessary(createIfNecessary), isMemoryDb(isMemoryDb), isEncryptedDb(isEncryptedDb),
          cipher(cipher), passwd(passwd)
    {
    }
#ifdef RELEASE_MODE_V3
    Option(bool createIfNecessary, bool isMemoryDb, bool isEncryptedDb, const DistributedDB::CipherType &cipher,
        const std::vector<uint8_t> &passwd, const DistributedDB::SecurityOption &secOption,
        DistributedDB::KvStoreObserver *observer, const DistributedDB::KvStoreNbConflictNotifier &notifier)
        : createIfNecessary(createIfNecessary), isMemoryDb(isMemoryDb), isEncryptedDb(isEncryptedDb),
          cipher(cipher), passwd(passwd), secOption(secOption), observer(observer), notifier(notifier)
    {
    }
#endif // endif of RELEASE_MODE_V3
    Option()
    {}
};

struct ConflictData {
    DistributedDB::KvStoreNbConflictType type;
    DistributedDB::Key key;
    DistributedDB::Value oldValue;
    DistributedDB::Value newValue;
    bool oldIsDeleted = false;
    bool newIsDeleted = false;
    bool oldIsNative = false;
    bool newIsNative = false;
};

enum EntryType {
    INSERT_LOCAL = 0,
    INSERT_NATIVE = 1,
    DELETE_LOCAL = 2,
    DELETE_NATIVE = 3,
    UPDATE_LOCAL = 4,
    UPDATE_NATIVE = 5
};

enum class ReadOrWriteTag {
    READ = 0,
    WRITE = 1,
    DELETE = 2,
    REGISTER = 3
};

const static DBParameters g_dbParameter1(DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);
const static DBParameters g_dbParameter2(DistributedDBDataGenerator::STORE_ID_2,
    DistributedDBDataGenerator::APP_ID_2, DistributedDBDataGenerator::USER_ID_2);
const static DBParameters g_dbParameter3(DistributedDBDataGenerator::STORE_ID_3,
    DistributedDBDataGenerator::APP_ID_3, DistributedDBDataGenerator::USER_ID_3);
const static DBParameters g_dbParameter4(DistributedDBDataGenerator::STORE_ID_4,
    DistributedDBDataGenerator::APP_ID_4, DistributedDBDataGenerator::USER_ID_4);
const static DBParameters g_dbParameter5(DistributedDBDataGenerator::STORE_ID_5,
    DistributedDBDataGenerator::APP_ID_5, DistributedDBDataGenerator::USER_ID_5);
const static DBParameters g_dbParameter6(DistributedDBDataGenerator::STORE_ID_6,
    DistributedDBDataGenerator::APP_ID_6, DistributedDBDataGenerator::USER_ID_6);
const static DBParameters g_dbParameter2_1(DistributedDBDataGenerator::STORE_ID_2,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);
const static DBParameters DB_PARAMETER_0_1(DistributedDBDataGenerator::STORE_ID,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);
const static DBParameters g_dbParameter1_2_1(DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_2, DistributedDBDataGenerator::USER_ID_1);
const static DBParameters g_dbParameter2_1_2(DistributedDBDataGenerator::STORE_ID_2,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_2);
const static DBParameters g_dbParameter3_2_1(DistributedDBDataGenerator::STORE_ID_3,
    DistributedDBDataGenerator::APP_ID_2, DistributedDBDataGenerator::USER_ID_1);
const static DBParameters g_dbParameter4_2_2(DistributedDBDataGenerator::STORE_ID_4,
    DistributedDBDataGenerator::APP_ID_2, DistributedDBDataGenerator::USER_ID_2);
const static Option g_createDiskUnencrypted(true, false, false, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::NULL_PASSWD_VECTOR);
const static Option g_createDiskEncrypted(true, false, true, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::PASSWD_VECTOR_1);
const static Option g_ncreateDiskUnencrypted(false, false, false, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::NULL_PASSWD_VECTOR);
const static Option g_ncreateDiskEncrypted(false, false, true, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::PASSWD_VECTOR_1);
const static Option g_createMemUnencrypted(true, true, false, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::NULL_PASSWD_VECTOR);
static Option g_option = g_createDiskUnencrypted;
const std::vector<Option> g_nbOptions = {g_createDiskUnencrypted, g_createDiskEncrypted, g_createMemUnencrypted};
// DelegateMgrNbCallback conclude the Callback implements of function< void(DBStatus, KvStoreDelegate*)>
class DelegateMgrNbCallback {
public:
    DelegateMgrNbCallback() {}
    ~DelegateMgrNbCallback() {}

    // Delete the copy and assign constructors
    DelegateMgrNbCallback(const DelegateMgrNbCallback &callback) = delete;
    DelegateMgrNbCallback& operator=(const DelegateMgrNbCallback &callback) = delete;
    DelegateMgrNbCallback(DelegateMgrNbCallback &&callback) = delete;
    DelegateMgrNbCallback& operator=(DelegateMgrNbCallback &&callback) = delete;

    void Callback(DistributedDB::DBStatus status, DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate);
    DistributedDB::DBStatus GetStatus();
    DistributedDB::KvStoreNbDelegate *GetKvStore();

private:
    DistributedDB::DBStatus status_ = DistributedDB::DBStatus::INVALID_ARGS;
    DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate_ = nullptr;
};

class ConflictNbCallback {
public:
    ConflictNbCallback() {}
    ~ConflictNbCallback() {}

    // Delete the copy and assign constructors
    ConflictNbCallback(const ConflictNbCallback &callback) = delete;
    ConflictNbCallback &operator=(const ConflictNbCallback &callback) = delete;
    ConflictNbCallback(ConflictNbCallback &&callback) = delete;
    ConflictNbCallback &operator=(ConflictNbCallback &&callback) = delete;

    void NotifyCallBack(const DistributedDB::KvStoreNbConflictData &data, std::vector<ConflictData> *&conflictData);
};

class DistributedDBNbTestTools final {
public:
    DistributedDBNbTestTools() {}
    ~DistributedDBNbTestTools() {}

    // Delete the copy and assign constructors
    DistributedDBNbTestTools(const DistributedDBNbTestTools &distributedDBNbTestTools) = delete;
    DistributedDBNbTestTools& operator=(const DistributedDBNbTestTools &distributedDBNbTestTools) = delete;
    DistributedDBNbTestTools(DistributedDBNbTestTools &&distributedDBNbTestTools) = delete;
    DistributedDBNbTestTools& operator=(DistributedDBNbTestTools &&distributedDBNbTestTools) = delete;
    static DistributedDB::KvStoreNbDelegate* GetNbDelegateStatus(DistributedDB::KvStoreDelegateManager *&outManager,
        DistributedDB::DBStatus &statusReturn, const DBParameters &param, const Option &optionParam);
    static DistributedDB::KvStoreNbDelegate* GetNbDelegateSuccess(DistributedDB::KvStoreDelegateManager *&outManager,
        const DBParameters &param, const Option &optionParam,
        const std::string &dbPath = DistributedDBDataGenerator::DistributedDBConstant::NB_DIRECTOR);
    static DistributedDB::DBStatus GetNbDelegateStoresSuccess(DistributedDB::KvStoreDelegateManager *&outManager,
        std::vector<DistributedDB::KvStoreNbDelegate *> &outDelegateVec, const std::vector<std::string> &storeIds,
        const std::string &appId, const std::string &userId, const Option &optionParam);
    static DistributedDB::KvStoreNbDelegate::Option TransferNbOptionType(const Option &optionParam);
    // this static method is to compare if the two Value has the same data.
    static bool isValueEquals(const DistributedDB::Value &v1, const DistributedDB::Value &v2);

    static DistributedDB::DBStatus Get(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &key, DistributedDB::Value &value);

    static DistributedDB::DBStatus GetEntries(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &keyPrefix, std::vector<DistributedDB::Entry> &entries);

    static DistributedDB::DBStatus Put(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &key, const DistributedDB::Value &value, bool isNeedRetry = false, int waitTime = 100);

    static DistributedDB::DBStatus PutBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const std::vector<DistributedDB::Entry> &entries, bool isNeedRetry = false);

    static DistributedDB::DBStatus Delete(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &key);

    static DistributedDB::DBStatus DeleteBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const std::vector<DistributedDB::Key> &keys, bool isNeedRetry = false);

    static DistributedDB::DBStatus GetLocal(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &key, DistributedDB::Value &value);

    static DistributedDB::DBStatus PutLocal(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &key, const DistributedDB::Value &value, bool isNeedRetry = false, int waitTime = 100);

    static DistributedDB::DBStatus PutLocalBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const std::vector<DistributedDB::Entry> &entries);

    static DistributedDB::DBStatus DeleteLocal(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &key);

    static DistributedDB::DBStatus DeleteLocalBatch(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const std::vector<DistributedDB::Key> &keys);

    static DistributedDB::DBStatus RegisterObserver(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::Key &key, unsigned int mode, DistributedDB::KvStoreObserver *observer);

    static DistributedDB::DBStatus UnRegisterObserver(DistributedDB::KvStoreNbDelegate &kvStoreNbDelegate,
        const DistributedDB::KvStoreObserver *observer);
    static bool CloseNbAndRelease(DistributedDB::KvStoreDelegateManager *&manager,
        DistributedDB::KvStoreNbDelegate *&delegate);
    static bool ModifyDatabaseFile(const std::string &fileDir);
    static std::string GetKvNbStoreDirectory(const DBParameters &param,
        const std::string &dbFilePath = DistributedDBDataGenerator::DistributedDBConstant::NB_DATABASE_NAME,
        const std::string &dbDir = DistributedDBDataGenerator::DistributedDBConstant::NB_DIRECTOR);
    static bool MoveToNextFromBegin(DistributedDB::KvStoreResultSet &resultSet,
        const std::vector<DistributedDB::Entry> &entries, int recordCnt);
    static std::string FbfFileToSchemaString(const std::string &fbFileDir, const std::string &fileName);
    static bool GetCurrentDir(std::string &dir);
    static std::string GetResourceDir();
    static bool CheckNbNoRecord(DistributedDB::KvStoreNbDelegate *&delegate, const DistributedDB::Key &key,
        bool bIsLocalQuery = false);
    static bool CheckNbRecord(DistributedDB::KvStoreNbDelegate *&delegate,
        const DistributedDB::Key &key, const DistributedDB::Value &value, bool bIsLocalQuery = false);
};

struct CallBackParam {
    const std::string path = "";
    bool result = true;
};

class KvStoreNbCorruptInfo {
public:
    KvStoreNbCorruptInfo() {}
    ~KvStoreNbCorruptInfo() {}

    KvStoreNbCorruptInfo(const KvStoreNbCorruptInfo&) = delete;
    KvStoreNbCorruptInfo& operator=(const KvStoreNbCorruptInfo&) = delete;
    KvStoreNbCorruptInfo(KvStoreNbCorruptInfo&&) = delete;
    KvStoreNbCorruptInfo& operator=(KvStoreNbCorruptInfo&&) = delete;

    // callback function will be called when the db data is changed.
    void CorruptCallBack(const std::string &appId, const std::string &userId, const std::string &storeId,
        bool &isCalled)
    {
        isCalled = true;
        MST_LOG("The corrupt Db is %s, %s, %s", appId.c_str(), userId.c_str(), storeId.c_str());
    }
    void CorruptNewCallBack(const std::string &appId, const std::string &userId, const std::string &storeId,
        DistributedDB::KvStoreDelegateManager *&manager, CallBackParam &param);
    void CorruptCallBackOfImport(const std::string &appId, const std::string &userId, const std::string &storeId,
        DistributedDB::KvStoreNbDelegate *&delegate, CallBackParam &param);
    void CorruptCallBackOfExport(const std::string &appId, const std::string &userId, const std::string &storeId,
        DistributedDB::KvStoreNbDelegate *&delegate, CallBackParam &param);
};
#ifdef RELEASE_MODE_V2
template <typename ParaType>
class QueryGenerate {
public:
    static constexpr int FP_COMP_SIZE = 6;
    static constexpr int FP_LIKE_SIZE = 2;
    static constexpr int FP_IN_SIZE = 2;

    typedef DistributedDB::Query&(DistributedDB::Query::*FP_COMP)(const std::string& field, const ParaType& value);
    typedef DistributedDB::Query&(DistributedDB::Query::*FP_LIKE)(const std::string& field, const std::string& value);
    typedef DistributedDB::Query&(DistributedDB::Query::*FP_IN)(const std::string& field,
        const std::vector<ParaType>& value);

    static QueryGenerate& Instance()
    {
        static QueryGenerate queryGenerateSingleton;
        return queryGenerateSingleton;
    }

    void GenerateQueryComp(std::vector<DistributedDB::Query> &queries, const bool (&funcFilter)[FP_COMP_SIZE],
        const std::string& field, const ParaType& value)
    {
        for (int i = 0; i < FP_COMP_SIZE; i++) {
            if (!funcFilter[i]) {
                continue;
            }
            queries.push_back((DistributedDB::Query::Select().*(compFunc_[i]))(field, value));
        }
    }

    void GenerateQueryLike(std::vector<DistributedDB::Query> &queries, const bool (&funcFilter)[FP_LIKE_SIZE],
        const std::string& field, const std::string& value)
    {
        for (int i = 0; i < FP_LIKE_SIZE; i++) {
            if (!funcFilter[i]) {
                continue;
            }
            queries.push_back((DistributedDB::Query::Select().*(likeFunc_[i]))(field, value));
        }
    }
    void GenerateQueryIn(std::vector<DistributedDB::Query> &queries, const bool (&funcFilter)[FP_IN_SIZE],
        const std::string& field, const std::vector<ParaType>& value)
    {
        for (int i = 0; i < FP_IN_SIZE; i++) {
            if (!funcFilter[i]) {
                continue;
            }
            queries.push_back((DistributedDB::Query::Select().*(inFunc_[i]))(field, value));
        }
    }
private:
    std::vector<FP_COMP> compFunc_{
        &DistributedDB::Query::EqualTo,
        &DistributedDB::Query::NotEqualTo,
        &DistributedDB::Query::GreaterThan,
        &DistributedDB::Query::GreaterThanOrEqualTo,
        &DistributedDB::Query::LessThan,
        &DistributedDB::Query::LessThanOrEqualTo,
    };
    std::vector<FP_LIKE> likeFunc_{
        &DistributedDB::Query::Like,
        &DistributedDB::Query::NotLike,
    };
    std::vector<FP_IN> inFunc_{
        &DistributedDB::Query::In,
        &DistributedDB::Query::NotIn,
    };
};
#endif // endif of RELEASE_MODE_V2

    bool EndCaseDeleteDB(DistributedDB::KvStoreDelegateManager *&manager, DistributedDB::KvStoreNbDelegate *&nbDelegate,
        const std::string &base, bool isMemoryDb);
#endif // DISTRIBUTEDDB_NB_TEST_TOOLS_H