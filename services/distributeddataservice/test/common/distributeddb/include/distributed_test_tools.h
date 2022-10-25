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
#ifndef DISTRIBUTED_DB_MODULE_TEST_TOOLS_H
#define DISTRIBUTED_DB_MODULE_TEST_TOOLS_H
#include <condition_variable>
#include <thread>
#include <vector>

#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_observer_impl.h"
#include "distributed_test_sysinfo.h"
#include "distributeddb_data_generator.h"
#include "log_print.h"
#ifdef RUN_MST_ON_TRUNCK // only need it in WAGNER env if run MST
#define HWTEST_F(test_case_name, test_name, level) TEST_F(test_case_name, test_name)
#endif
#define ULL(x) (static_cast<unsigned long long>(x))
const int MAX_DIR_LENGTH = 4096; // the max length of directory
const int BUF_LEN = 8192;
const static std::string TAG = "DistributedTestTools"; // for log
const int AUTHORITY = 0755;
const int E_OK = 0;
const int E_ERROR = -1;
const std::string DIRECTOR = "/data/test/getstub/"; // default work dir.
static std::condition_variable g_conditionKvVar;

struct KvDBParameters {
    std::string storeId;
    std::string appId;
    std::string userId;
    KvDBParameters(std::string storeIdStr, std::string appIdStr, std::string userIdStr)
        : storeId(storeIdStr), appId(appIdStr), userId(userIdStr)
    {
    }
};

struct KvOption {
    bool createIfNecessary = true;
    bool localOnly = false;
    bool isEncryptedDb = false; // whether need encrypt
    DistributedDB::CipherType cipher = DistributedDB::CipherType::DEFAULT; // cipher type
    std::vector<uint8_t> passwd; // cipher password
    KvOption(bool createIfNecessary1, bool isLocalOnly,
    bool isEncryptedDb1, DistributedDB::CipherType cipher1, std::vector<uint8_t> passwd1)
        : createIfNecessary(createIfNecessary1),
          localOnly(isLocalOnly),
          isEncryptedDb(isEncryptedDb1),
          cipher(cipher1), passwd(passwd1)
    {
    }
    KvOption()
    {}
};

struct EncrypteAttribute {
    bool isEncryptedDb = false;
    std::vector<uint8_t> passwd;
};

const static KvDBParameters g_kvdbParameter1(DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);
const static KvDBParameters g_kvdbParameter2(DistributedDBDataGenerator::STORE_ID_2,
    DistributedDBDataGenerator::APP_ID_2, DistributedDBDataGenerator::USER_ID_2);
const static KvDBParameters g_kvdbParameter3(DistributedDBDataGenerator::STORE_ID_3,
    DistributedDBDataGenerator::APP_ID_3, DistributedDBDataGenerator::USER_ID_3);
const static KvDBParameters g_kvdbParameter4(DistributedDBDataGenerator::STORE_ID_4,
    DistributedDBDataGenerator::APP_ID_4, DistributedDBDataGenerator::USER_ID_4);
const static KvDBParameters g_kvdbParameter5(DistributedDBDataGenerator::STORE_ID_5,
    DistributedDBDataGenerator::APP_ID_5, DistributedDBDataGenerator::USER_ID_5);
const static KvDBParameters g_kvdbParameter6(DistributedDBDataGenerator::STORE_ID_6,
    DistributedDBDataGenerator::APP_ID_6, DistributedDBDataGenerator::USER_ID_6);
const static KvDBParameters g_kvdbParameter1_1_2(DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_2);
const static KvDBParameters g_kvdbParameter1_2_1(DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_2, DistributedDBDataGenerator::USER_ID_1);
const static KvDBParameters g_kvdbParameter1_2_2(DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_2, DistributedDBDataGenerator::USER_ID_2);
const static KvDBParameters g_kvdbParameter2_1_1(DistributedDBDataGenerator::STORE_ID_2,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);
const static KvDBParameters g_kvdbParameter2_1_2(DistributedDBDataGenerator::STORE_ID_2,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_2);
const static KvDBParameters KVDB_PARAMETER_PERFORM(DistributedDBDataGenerator::STORE_ID_PERFORM,
    DistributedDBDataGenerator::APP_ID_PERFORM, DistributedDBDataGenerator::USER_ID_PERFORM);
const static KvOption g_createKvDiskUnencrypted(true, false, false, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::NULL_PASSWD_VECTOR);
const static KvOption g_createKvDiskEncrypted(true, false, true, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::PASSWD_VECTOR_1);
const static KvOption g_createLocalDiskUnencrypted(true, true, false, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::NULL_PASSWD_VECTOR);
const static KvOption g_createLocalDiskEncrypted(true, true, true, DistributedDB::CipherType::DEFAULT,
    DistributedDBDataGenerator::PASSWD_VECTOR_1);
static KvOption g_kvOption = g_createKvDiskEncrypted;
bool CompareVector(const std::vector<uint8_t>& first, const std::vector<uint8_t>& second);
bool CompareEntriesVector(std::vector<DistributedDB::Entry>& retVec,
    std::vector<DistributedDB::Entry>& expectVec);
void PutUniqueKey(std::vector<DistributedDB::Entry>& entryVec,
    const DistributedDB::Key &putKey, const DistributedDB::Value &putValue);
int Uint8VecToString(std::vector<uint8_t>& vec, std::string& str);
int GetIntValue(DistributedDB::Value &value);
int RemoveDir(const std::string &directory);
int SetDir(const std::string &directory, const int authRight = AUTHORITY);
void CopyFile(const std::string &srcFile, const std::string &destFile);
void CopyDir(const std::string &srcDir, const std::string &destDir, const int authRight = AUTHORITY);
void CheckFileNumber(const std::string &filePath, int &fileCount);
DistributedDB::Value GetValueWithInt(int val);
std::vector<DistributedDB::Entry> GenRanKeyVal(int putGetTimes, int keyLength, int valueLength, char val);
std::vector<DistributedDB::Key> GetKeysFromEntries(std::vector<DistributedDB::Entry> entries, bool random);
bool GetRandBool();
bool PutEntries(DistributedDB::KvStoreNbDelegate *&delegate, const std::vector<DistributedDB::Entry> &entries);

using SysTime = std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>;
using SysDurTime = std::chrono::duration<uint64_t, std::micro>;

struct PerformanceData {
    int putGetTimes;
    int keyLength;
    int valueLength;
    bool putBatch;
    bool getBatch;
    bool useClear;
    bool getSysInfo;
    bool isLocal;
    double openDuration;
    double putDuration;
    double readPutDuration;
    double updateDuration;
    double readUpdateDuration;
    double deleteDuration;
    double closeDuration;
    PerformanceData(int putGetTimes, int keyLength, int valueLength,
        bool putBatch, bool getBatch, bool useClear, bool getSysInfo, bool isLocal)
        : putGetTimes(putGetTimes), keyLength(keyLength), valueLength(valueLength),
          putBatch(putBatch), getBatch(getBatch), useClear(useClear),
          getSysInfo(getSysInfo), isLocal(isLocal),
          openDuration(0.0), putDuration(0.0), readPutDuration(0.0), updateDuration(0.0),
          readUpdateDuration(0.0), deleteDuration(0.0), closeDuration(0.0)
    {
    }
};

struct Duration {
    double putDuration = 0.0;
    double readDuration = 0.0;
    double updateDuration = 0.0;
    double deleteDuration = 0.0;
    Duration() = default;
    void Clear()
    {
        putDuration = readDuration = updateDuration = deleteDuration = 0.0;
    }
};

struct BackupDuration {
    double exportDuration = 0.0;
    double importDuration = 0.0;
    BackupDuration() = default;
    BackupDuration(double exportDur, double importDur)
    {
        exportDuration = exportDur;
        importDuration = importDur;
    }
    BackupDuration operator+(const BackupDuration &backupDur) const
    {
        return BackupDuration(exportDuration + backupDur.exportDuration, importDuration + backupDur.importDuration);
    }
    void Clear()
    {
        exportDuration = importDuration = 0;
    }
};

enum KvDbType {
    ENCRYED = 0,
    UNENCRYED = 1
};

enum OperRecordNum {
    SINGLE = 1,
    SMALL_BATCH = 100,
    BATCH = 128,
};

enum class OperType {
    PUT,
    PUT_LOCAL,
    UPDATE,
    UPDATE_LOCAL,
    DELETE,
    DELETE_LOCAL
};

struct KvPerfData {
    KvDbType kvDbType;
    int testCnt;
    OperRecordNum operRecordNum;
    int keyLength;
    int valueLength;
    bool isPresetRecords;
    int presetRecordsCnt;
    std::vector<Duration> allCrudDur;
    KvPerfData(KvDbType kvDbType, int testCnt, OperRecordNum operRecordNum, int keyLength, int valueLength,
        bool isPresetRecords, int presetRecordsCnt)
        : kvDbType(kvDbType), testCnt(testCnt), operRecordNum(operRecordNum),
          keyLength(keyLength), valueLength(valueLength),
          isPresetRecords(isPresetRecords), presetRecordsCnt(presetRecordsCnt)
    {
    }
};

struct RekeyTypesDur {
    double nullPasswdToPasswd1 = 0.0;
    double passwd1ToPasswd1 = 0.0;
    double passwd1ToPasswd2 = 0.0;
    double passwd2ToNullPasswd = 0.0;
    RekeyTypesDur() = default;
    RekeyTypesDur(double nullPasswdToPasswd1, double passwd1ToPasswd1,
        double passwd1ToPasswd2, double passwd2ToNullPasswd)
        : nullPasswdToPasswd1(nullPasswdToPasswd1), passwd1ToPasswd1(passwd1ToPasswd1),
          passwd1ToPasswd2(passwd1ToPasswd2), passwd2ToNullPasswd(passwd2ToNullPasswd)
    {
    }
    RekeyTypesDur operator+(const RekeyTypesDur &rekeyTypesDur) const
    {
        return RekeyTypesDur(nullPasswdToPasswd1 + rekeyTypesDur.nullPasswdToPasswd1,
            passwd1ToPasswd1 + rekeyTypesDur.passwd1ToPasswd1,
            passwd1ToPasswd2 + rekeyTypesDur.passwd1ToPasswd2,
            passwd2ToNullPasswd + rekeyTypesDur.passwd2ToNullPasswd);
    }
};

struct KvRekeyPerfData {
    int testCnt;
    int presetRecordsCnt;
    int keyLength;
    int valueLength;
    std::vector<RekeyTypesDur> allRekeyDur;
    KvRekeyPerfData(int testCnt, int presetRecordsCnt, int keyLength, int valueLength)
        : testCnt(testCnt), presetRecordsCnt(presetRecordsCnt), keyLength(keyLength), valueLength(valueLength)
    {
    }
};

struct KvBackupPerfData {
    KvDbType kvDbType;
    int testCnt;
    int presetRecordsCnt;
    int keyLength;
    int valueLength;
    std::vector<BackupDuration> allBackupDur;
    KvBackupPerfData(KvDbType kvDbType, int testCnt, int presetRecordsCnt, int keyLength, int valueLength)
        : kvDbType(kvDbType), testCnt(testCnt), presetRecordsCnt(presetRecordsCnt),
          keyLength(keyLength), valueLength(valueLength)
    {
    }
};

struct PerImageGallery {
    uint64_t putDuration = 0;
    uint64_t readDuration = 0;
    void Clear()
    {
        putDuration = 0;
        readDuration = 0;
    }
};
struct NbGalleryPerfData {
    int testCnt;
    unsigned int keyLength;
    unsigned int valueLength;
    bool isLocal;
    bool isPresetRecords;
    int presetRecordsCnt;
    std::vector<PerImageGallery> allCrudDur;
    NbGalleryPerfData(int testCnt, int keyLength, int valueLength,
        bool isLocal, bool isPresetRecords, int presetRecordsCnt)
        : testCnt(testCnt), keyLength(keyLength), valueLength(valueLength),
          isLocal(isLocal), isPresetRecords(isPresetRecords), presetRecordsCnt(presetRecordsCnt)
    {
    }
};
struct NbLocalPerfData {
    int testCnt;
    unsigned int keyLength;
    unsigned int valueLength;
    bool isLocal;
    bool isPresetRecords;
    int presetRecordsCnt;
    std::vector<Duration> allCrudDur;
    NbLocalPerfData(int testCnt, int keyLength, int valueLength,
        bool isLocal, bool isPresetRecords, int presetRecordsCnt)
        : testCnt(testCnt), keyLength(keyLength), valueLength(valueLength),
          isLocal(isLocal), isPresetRecords(isPresetRecords), presetRecordsCnt(presetRecordsCnt)
    {
    }
};

struct QueryDur {
    double getEntriesDuration = 0.0;
    double getResultSetDuration = 0.0;
    QueryDur() = default;
    void Clear()
    {
        getEntriesDuration = getResultSetDuration = 0;
    }
};
struct NbSchemaCRUDPerfData {
    int testCnt;
    OperRecordNum operRecordNum;
    unsigned int keyLength;
    unsigned int valueLength;
    bool isLocal;
    bool isPresetRecords;
    unsigned int presetRecordsCnt;
    bool isQueryNeeded;
    bool isIndexSchema;
    std::vector<Duration> allCrudDur;
    std::vector<QueryDur> allQueryDur;
    NbSchemaCRUDPerfData(int testCnt, OperRecordNum operRecordNum, int keyLength, int valueLength,
        bool isLocal, bool isPresetRecords, int presetRecordsCnt, int isQueryNeeded, int isIndexSchema)
        : testCnt(testCnt), operRecordNum(operRecordNum), keyLength(keyLength), valueLength(valueLength),
          isLocal(isLocal), isPresetRecords(isPresetRecords), presetRecordsCnt(presetRecordsCnt),
          isQueryNeeded(isQueryNeeded), isIndexSchema(isIndexSchema)
    {
    }
};

// default kvStoreDelegateManager's config.
const DistributedDB::KvStoreConfig KV_CONFIG = {
    .dataDir = DIRECTOR
};

class DistributedTestTools final {
public:
    DistributedTestTools() {}
    ~DistributedTestTools() {}

    // Delete the copy and assign constructors
    DistributedTestTools(const DistributedTestTools &distributeDBTools) = delete;
    DistributedTestTools& operator=(const DistributedTestTools &distributeDBTools) = delete;
    DistributedTestTools(DistributedTestTools &&distributeDBTools) = delete;
    DistributedTestTools& operator=(DistributedTestTools &&distributeDBTools) = delete;

    // this static method is to compare if the two Value has the same data.
    static bool IsValueEquals(const DistributedDB::Value &v1, const DistributedDB::Value &v2);
    static DistributedDB::KvStoreDelegate::Option TransferKvOptionType(const KvOption &optionParam);
    static DistributedDB::KvStoreDelegate* GetDelegateSuccess(DistributedDB::KvStoreDelegateManager *&outManager,
        const KvDBParameters &param, const KvOption &optionParam);
    static DistributedDB::KvStoreDelegate* GetDelegateStatus(DistributedDB::KvStoreDelegateManager *&outManager,
        DistributedDB::DBStatus &status, const KvDBParameters &param, const KvOption &optionParam);

    static DistributedDB::DBStatus GetDelegateNotGood(
        DistributedDB::KvStoreDelegateManager *&outManager, DistributedDB::KvStoreDelegate *&outDelegate,
        const std::string &storeId, const std::string &appId, const std::string &userId, const KvOption &optionParam);

    static DistributedDB::DBStatus Put(DistributedDB::KvStoreDelegate &kvStoreDelegate,
        const DistributedDB::Key &key, const DistributedDB::Value &value);

    static DistributedDB::DBStatus PutBatch(DistributedDB::KvStoreDelegate &kvStoreDelegate,
        const std::vector<DistributedDB::Entry> &entries);

    static DistributedDB::DBStatus Delete(DistributedDB::KvStoreDelegate &kvStoreDelegate,
        const DistributedDB::Key &key);

    static DistributedDB::DBStatus DeleteBatch(DistributedDB::KvStoreDelegate &kvStoreDelegate,
        const std::vector<DistributedDB::Key> &keys);

    static DistributedDB::DBStatus Clear(DistributedDB::KvStoreDelegate &kvStoreDelegate);

    static DistributedDB::KvStoreSnapshotDelegate *GetKvStoreSnapshot(DistributedDB::KvStoreDelegate &kvStoreDelegate);
    static DistributedDB::Value Get(DistributedDB::KvStoreDelegate &kvStoreDelegate, const DistributedDB::Key &key);
    static DistributedDB::Value Get(DistributedDB::KvStoreSnapshotDelegate &kvStoreSnapshotDelegate,
        const DistributedDB::Key &key);

    static std::vector<DistributedDB::Entry> GetEntries(
        DistributedDB::KvStoreSnapshotDelegate &kvStoreSnapshotDelegate, const DistributedDB::Key &key);
    static std::vector<DistributedDB::Entry> GetEntries(DistributedDB::KvStoreDelegate &kvStoreDelegate,
        const DistributedDB::Key &keyPrefix);

    static DistributedDB::KvStoreSnapshotDelegate *RegisterSnapObserver(DistributedDB::KvStoreDelegate *delegate,
        DistributedDB::KvStoreObserver *observer);
    static DistributedDB::DBStatus RegisterObserver(DistributedDB::KvStoreDelegate *delegate,
        DistributedDB::KvStoreObserver *observer);
    static DistributedDB::DBStatus UnRegisterObserver(DistributedDB::KvStoreDelegate *delegate,
        DistributedDB::KvStoreObserver *observer);
    static bool CalculateOpenPerformance(PerformanceData &performanceData);
    static bool CalculateInsertPerformance(PerformanceData &performanceData);
    static bool CalculateGetPutPerformance(PerformanceData &performanceData);
    static bool CalculateUpdatePerformance(PerformanceData &performanceData);
    static bool CalculateGetUpdatePerformance(PerformanceData &performanceData);
    static bool CalculateUseClearPerformance(PerformanceData &performanceData);
    static bool CalculateTransactionPerformance(PerformanceData &performanceData);
    static bool CloseAndRelease(DistributedDB::KvStoreDelegateManager *&manager,
        DistributedDB::KvStoreDelegate *&delegate);
    static bool CloseAndRelease(DistributedDB::KvStoreDelegateManager *&manager,
        DistributedDB::KvStoreNbDelegate *&delegate);
    static bool VerifyDbRecordCnt(DistributedDB::KvStoreNbDelegate *&delegate, unsigned int recordCnt,
        bool isLocal = false);
    static bool VerifyRecordsInDb(DistributedDB::KvStoreNbDelegate *&delegate,
        std::vector<DistributedDB::Entry> &entriesExpected,
        const std::vector<uint8_t> &keyPrefix = DistributedDBDataGenerator::KEY_EMPTY, bool isLocal = false);
    static bool GetRecordCntByKey(const std::string &dbName,
        const std::string &strSql, std::vector<DistributedDB::Key> &sqlParam, KvOption &option, int &count);
    static bool QuerySpecifiedData(const std::string &dbName, const std::string &strSql,
        EncrypteAttribute &attribute, int &count);
    static bool RepeatCheckAsyncResult(const std::function<bool(void)> &inPred, int repeatLimit,
        uint32_t repeatInterval);
    static bool CompareKey(const DistributedDB::Entry &entry1, const DistributedDB::Entry &entry2);
    static void CopyFile(const std::string &srcFile, const std::string &destFile);
};

std::string TransferStringToHashHexString(const std::string &origStr);

int RemoveDatabaseDirectory(const std::string &directory);

bool VerifyObserverResult(const KvStoreObserverImpl &pObserver,
    int changedTimes, ListType type, const std::list<DistributedDB::Entry> &lst,
    uint32_t timeout = DistributedDBDataGenerator::DistributedDBConstant::THIRTY_MINUTES);
bool VerifyObserverResult(const KvStoreObserverImpl &pObserver,
    int changedTimes, ListType type, const std::vector<DistributedDB::Entry> &vec,
    uint32_t timeout = DistributedDBDataGenerator::DistributedDBConstant::THIRTY_MINUTES);
bool VerifyObserverForSchema(const KvStoreObserverImpl &pObserver,
    int changedTimes, ListType type, const std::vector<DistributedDB::Entry> &expectEntry,
    uint32_t timeout = DistributedDBDataGenerator::DistributedDBConstant::THIRTY_MINUTES);
#endif // DISTRIBUTED_DB_MODULE_TEST_TOOLS_H
