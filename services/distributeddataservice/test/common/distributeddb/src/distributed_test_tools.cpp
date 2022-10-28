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
#include "distributed_test_tools.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <openssl/sha.h>
#include <random>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifndef USE_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif
#include "delegate_callback.h"
#include "delegate_kv_mgr_callback.h"
#include "distributeddb_data_generator.h"
#include "distributed_test_sysinfo.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "platform_specific.h"
#include "securec.h"

using namespace std;
using namespace chrono;
using namespace std::placeholders;
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

namespace {
#ifndef USE_SQLITE_CODEC_CIPHER
    const std::string CIPHER_CONFIG_SQL = "PRAGMA cipher='aes-256-gcm';";
    const std::string KDF_ITER_CONFIG_SQL = "PRAGMA kdf_iter=5000;";
#else
    const std::string CIPHER_CONFIG_SQL = "PRAGMA codec_cipher='aes-256-gcm';";
    const std::string KDF_ITER_CONFIG_SQL = "PRAGMA codec_kdf_iter=5000;";
#endif
    const int RAND_BOOL_MIN = 0;
    const int RAND_BOOL_MAX = 1;
}

bool CompareVector(const std::vector<uint8_t>& first, const std::vector<uint8_t>& second)
{
    if (first.size() != second.size()) {
        MST_LOG("[CompareVector] first.size[%zu] != second.size[%zu]", first.size(), second.size());
        return false;
    }
    return first == second;
}

bool CompareEntriesVector(std::vector<DistributedDB::Entry>& retVec,
    std::vector<DistributedDB::Entry>& expectVec)
{
    if (retVec.size() != expectVec.size()) {
        MST_LOG("retVec.size() = %zd, expectVec.size() = %zd", retVec.size(), expectVec.size());
        return false;
    }

    // for retVec and expectVec are reference, don't want to change the value of them
    vector<std::reference_wrapper<Entry>> retVector(retVec.begin(), retVec.end());
    vector<std::reference_wrapper<Entry>> expVec(expectVec.begin(), expectVec.end());
    sort(retVector.begin(), retVector.end(), DistributedTestTools::CompareKey);
    sort(expVec.begin(), expVec.end(), DistributedTestTools::CompareKey);
    for (uint64_t at = 0; at < retVector.size(); at++) {
        bool result = ((retVector[at].get().key == expVec[at].get().key) &&
            (retVector[at].get().value == expVec[at].get().value));
        if (!result) {
            MST_LOG("[CompareEntriesVector] compare list failed at the position: %llu", ULL(at));
            string retKey(retVector[at].get().key.begin(), retVector[at].get().key.end());
            string retValue(retVector[at].get().value.begin(), retVector[at].get().value.end());
            string expKey(expVec[at].get().key.begin(), expVec[at].get().key.end());
            string expValue(expVec[at].get().value.begin(), expVec[at].get().value.end());
            MST_LOG("[CompareEntriesVector] the actual Key: %s\n", retKey.c_str());
            MST_LOG("[CompareEntriesVector] the expect key: %s\n", expKey.c_str());
            // retValue num lessThan 10, expValue num lessThan 10,
            if (retValue.size() > 10 && expValue.size() > 10) {
                MST_LOG("[CompareEntriesVector] the actual value: %s...%s\n",
                    // retValue substr num as 10, the index of retKey.size() - 10
                    retValue.substr(0, 10).c_str(), retValue.substr((retValue.size() - 10), 10).c_str());
                MST_LOG("[CompareEntriesVector] the expect value: %s...%s\n",
                    // expValue substr num as 10, the index of retKey.size() - 10
                    expValue.substr(0, 10).c_str(), expValue.substr((expValue.size() - 10), 10).c_str());
            } else {
                MST_LOG("[CompareEntriesVector] the actual value: %s\n", retValue.c_str());
                MST_LOG("[CompareEntriesVector] the expect value: %s\n", expValue.c_str());
            }
            return result;
        }
    }
    return true;
}

void PutUniqueKey(vector<Entry> &entryVec, const Key &putKey, const Value &putValue)
{
    bool findEachEntry = false;
    for (unsigned int idxEntry = 0; idxEntry < entryVec.size(); ++idxEntry) {
        if (CompareVector(entryVec[idxEntry].key, putKey)) {
            findEachEntry = true;
            entryVec[idxEntry].value = putValue;
            break;
        }
    }
    Entry entry = { putKey, putValue };
    if (!findEachEntry) {
        entryVec.push_back(entry);
    }
}

int Uint8VecToString(std::vector<uint8_t> &vec, std::string &str)
{
    int len = 0;
    str.clear();

    for (auto vecIt = vec.begin(); vecIt != vec.end(); ++vecIt) {
        str.push_back(static_cast<char>(*vecIt));
        ++len;
    }

    return len;
}

vector<Entry> GenRanKeyVal(int putGetTimes, int keyLength, int valueLength, char val)
{
    // random gen the key
    std::random_device randDevKeyNo;
    std::mt19937 genRandKeyNo(randDevKeyNo());
    std::uniform_int_distribution<uint8_t> disRandKeyNo(CHAR_SPAN_MIN, CHAR_SPAN_MAX);

    DistributedDB::Entry entryCurrent;
    vector<Entry> entriesBatch;

    for (int cnt = 0; cnt < putGetTimes; ++cnt) {
        DistributedDB::Key tempKey = PERFORMANCEKEY;
        for (int kIndex = 0; kIndex < keyLength; ++kIndex) {
            tempKey.push_back(disRandKeyNo(genRandKeyNo));
        }
        entryCurrent.key = tempKey;
        entryCurrent.value.assign(valueLength, val);

        entriesBatch.push_back(entryCurrent);
    }
    return entriesBatch;
}

bool GetRandBool()
{
    std::random_device randDev;
    std::mt19937 genRand(randDev());
    std::uniform_int_distribution<int> disRand(RAND_BOOL_MIN, RAND_BOOL_MAX);
    return disRand(genRand);
}

bool PutEntries(DistributedDB::KvStoreNbDelegate *&delegate, const std::vector< DistributedDB::Entry > &entries)
{
    for (const auto &entry : entries) {
        if (delegate->Put(entry.key, entry.value) != OK) {
            return false;
        }
    }
    return true;
}

vector<Key> GetKeysFromEntries(std::vector < DistributedDB::Entry > entries, bool random)
{
    vector<Key> result;
    vector<Entry> old;
    old.assign(entries.begin(), entries.end());
    if (random) {
        for (int i = entries.size(); i > 0; i--) {
            int index = GetRandInt(0, ONE_HUNDRED_SECONDS * MILLSECONDS_PER_SECOND) % i;
            result.push_back(old[index].key);
            old.erase(old.begin() + index);
        }
    } else {
        for (auto entry = entries.begin(); entry != entries.end(); ++entry) {
            result.push_back(entry->key);
        }
    }
    return result;
}

int GetIntValue(Value &value)
{
    if (value.size() == 0) {
        return 0;
    }
    string strVal;
    Uint8VecToString(value, strVal);
    int number;
    if (sscanf_s(strVal.c_str(), "%d", &number) != 1) {
        return 0;
    }
    return number;
}

Value GetValueWithInt(int val)
{
    string strVal = std::to_string(val);
    Value result;
    result.assign(strVal.begin(), strVal.end());
    return result;
}

vector< vector<Entry> > GetGroupEntries(vector<Entry> entries, int pageSize, int valueLength, uint8_t c)
{
    vector< vector<Entry> > result;
    if (pageSize <= 0) {
        MST_LOG("pageSize can't be zero or negative number!");
    } else {
        int pages = (entries.size() - 1) / pageSize + 1;
        for (int pageIndex = 0; pageIndex < pages; ++pageIndex) {
            vector<Entry> temp;
            int pageStart = pageSize * pageIndex;
            int pageEnd = (pageSize * (pageIndex + 1)) - 1;
            if (pageEnd + 1 > static_cast<int>(entries.size())) {
                pageEnd = entries.size() - 1;
            }
            MST_LOG("The %d page start position: %d, end position: %d", pageIndex, pageStart, pageEnd);
            temp.assign(entries.begin() + pageStart, entries.begin() + pageEnd);
            result.push_back(temp);
        }
        for (auto iter = result.begin(); iter != result.end(); ++iter) {
            for (auto entry = iter->begin(); entry != iter->end(); ++entry) {
                entry->value.assign(valueLength, c);
            }
        }
    }
    return result;
}

// KvCallback conclude the Callback of function <void(DBStatus, Value)> and <void(DBStatus, vector<Entry>)>
class KvCallback {
public:
    KvCallback() {}
    ~KvCallback() {}

    // Delete the copy and assign constructors
    KvCallback(const KvCallback &callback) = delete;
    KvCallback& operator=(const KvCallback &callback) = delete;
    KvCallback(KvCallback &&callback) = delete;
    KvCallback& operator=(KvCallback &&callback) = delete;

    void Callback(DBStatus status, const Value &value)
    {
        this->status_ = status;
        this->value_ = value;
    }

    void CallbackPrefix(DBStatus status, const vector<Entry> &entries)
    {
        this->status_ = status;
        this->entries_ = entries;
    }

    DBStatus GetStatus() const
    {
        return status_;
    }

    const Value &GetValue() const
    {
        return value_;
    }

    const vector<Entry> &GetValueVector() const
    {
        return entries_;
    }

private:
    DBStatus status_ = DBStatus::INVALID_ARGS;
    Value value_ = { };
    vector<Entry> entries_ = {};
};

// delete dir(directory) recursively
int RemoveDir(const std::string &dirSrc)
{
    string directory = dirSrc;
    char curDir[] = ".";
    char upDir[] = "..";
    char dirName[MAX_DIR_LENGTH] = { 0 };
    DIR *dirp = nullptr;
    struct dirent *dp = nullptr;
    struct stat dirStat;

    while (directory.at(directory.length() - 1) == '/') {
        directory.pop_back();
    }

    const char *dir = directory.c_str();
    // if dir is not existed
    if (access(dir, F_OK) != 0) {
        return 0;
    }
    // getting dir attribution fails
    if (stat(dir, &dirStat) < 0) {
        perror("get directory stat error");
        return -1;
    }
    // if dir is a common file
    if (S_ISREG(dirStat.st_mode)) {
        remove(dir);
    } else if (S_ISDIR(dirStat.st_mode)) {
        // if dir is directory, delete it recursively
        dirp = opendir(dir);
        if (dirp == nullptr) {
            perror("open directory error");
            return -1;
        }
        while ((dp = readdir(dirp)) != nullptr) {
            // ignore . and ..
            if ((strcmp(curDir, dp->d_name) == 0) || (strcmp(upDir, dp->d_name)) == 0) {
                continue;
            }
            int result = sprintf_s(dirName, sizeof(dirName) - 1, "%s/%s", dir, dp->d_name);
            if (result < 0) {
                closedir(dirp);
                return -1;
            }
            MST_LOG("delete dirName = %s", dirName);
            RemoveDir(std::string(dirName));
        }
        closedir(dirp);
        remove(directory.c_str());
    } else {
        perror("unknown file type!");
    }
    return 0;
}

// If the test cases' related dir in system is not exist, create it.
int SetDir(const std::string &directory, const int authRight)
{
    MST_LOG("create directory: %s", directory.c_str());
    char dir[MAX_DIR_LENGTH] = { 0 };
    int i, len;

    errno_t ret = strcpy_s(dir, MAX_DIR_LENGTH, directory.c_str());
    if (ret != E_OK) {
        MST_LOG("strcpy_s failed(%d)!", ret);
        return -1;
    }
    len = strlen(dir);
    if (dir[len - 1] != '/') {
        ret = strcat_s(dir, sizeof(dir) - strlen(dir) - 1, "/");
        if (ret != E_OK) {
            MST_LOG("strcat_s failed(%d)!", ret);
            return -1;
        }
        len++;
    }

    for (i = 1; i < len; i++) {
        if (dir[i] != '/') {
            continue;
        }
        dir[i] = '\0';
        if (access(dir, F_OK) != E_OK) {
            mode_t mode = umask(0);
#if defined(RUNNING_ON_LINUX)
            if (mkdir(dir, authRight) == E_ERROR) {
#else
            std::string stringDir = dir;
            if (OS::MakeDBDirectory(stringDir) == E_ERROR) {
#endif
                MST_LOG("mkdir(%s) failed(%d)!", dir, errno);
                return -1;
            }
            if (chdir(dir) == E_ERROR) {
                MST_LOG("chdir(%s) failed(%d)!", dir, errno);
            }
            umask(mode);
        }
        dir[i] = '/';
    }
    MST_LOG("check dir = %s", dir);
    return 0;
}

void CopyFile(const string &srcFile, const string &destFile)
{
    int len = 0;
    char buff[BUF_LEN] = { 0 };

    FILE *pIn = fopen(srcFile.c_str(), "r");
    if (pIn == nullptr) {
        perror("pIn");
        return;
    }

    FILE *pOut = fopen(destFile.c_str(), "w");
    if (pOut == nullptr) {
        perror("pOut");
        fclose(pIn);
        return;
    }

    while ((len = fread(buff, sizeof(char), sizeof(buff), pIn)) > 0) {
        fwrite(buff, sizeof(char), len, pOut);
    }
    fclose(pOut);
    fclose(pIn);
}

void CopyDir(const string &srcDir, const string &destDir, const int authRight)
{
    string srcFullDir = srcDir;
    string destFullDir = destDir;
    struct dirent *filename = nullptr;
    DIR *dpDest = opendir(destFullDir.c_str());
    if (dpDest == nullptr) {
        if (SetDir(destFullDir.c_str(), authRight)) {
            MST_LOG("[CopyDir] SetDir(%s) failed(%d)!", destFullDir.c_str(), errno);
            return;
        }
    } else {
        closedir(dpDest);
    }
    string path = srcFullDir;
    if (srcFullDir.back() != '/') {
        srcFullDir += "/";
    }
    if (destFullDir.back() != '/') {
        destFullDir += "/";
    }

    DIR *dpSrc = opendir(path.c_str());
    if (dpSrc == nullptr) {
        MST_LOG("[CopyDir] please make sure srcDir(%s) is valid.", srcDir.c_str());
        return;
    }
    while ((filename = readdir(dpSrc)) != nullptr) {
        string fileSourceDir = srcFullDir;
        string fileDestDir = destFullDir;

        fileSourceDir += filename->d_name;
        fileDestDir += filename->d_name;
        if (filename->d_type == DT_DIR) {
            if ((string(filename->d_name).compare(0, strlen("."), ".") != 0) &&
                (string(filename->d_name).compare(0, strlen(".."), "..") != 0)) {
                CopyDir(fileSourceDir, fileDestDir);
            }
        } else {
            CopyFile(fileSourceDir, fileDestDir);
        }
    }
    closedir(dpSrc);
    MST_LOG("[CopyDir] copy file from %s to %s successfully.", srcDir.c_str(), destDir.c_str());
}

void CheckFileNumber(const string &filePath, int &fileCount)
{
    std::string cmd = "cd " + filePath + ";ls -l |grep " + "^-" + "|wc -l";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (pipe == nullptr) {
        MST_LOG("Check file number filed.");
        return;
    }

    char buffer[128]; // set pipe buffer length as 128B
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) { // set pipe buffer length as 128B
            result += buffer;
        }
    }
    pclose(pipe);
    fileCount = std::stoi(result);
    MST_LOG("Check file number is %d", fileCount);
}

// this static method is to compare if the two Value has the same data.
bool DistributedTestTools::IsValueEquals(const DistributedDB::Value &v1, const DistributedDB::Value &v2)
{
    // just return false if the sizes are not the same.
    if (v1.size() != v2.size()) {
        return false;
    }

    // compare two Values char by char.
    return v1 == v2;
}

// This static method is for moduleTest of distributed to get KvStoreDelegate with valid ids.
KvStoreDelegate* DistributedTestTools::GetDelegateSuccess(KvStoreDelegateManager *&outManager,
    const KvDBParameters &param, const KvOption &optionParam)
{
    MST_LOG("GetDelegateSuccess isEncryptedDb= %d", optionParam.isEncryptedDb);
    SetDir(DIRECTOR);
    if (param.storeId.empty() || param.appId.empty() || param.userId.empty()) {
        return nullptr;
    }

    // define a Callback to hold the kvStoreDelegate and status.
    DelegateKvMgrCallback delegateKvMgrCallback;
    function<void(DBStatus, KvStoreDelegate*)> callFunction
        = bind(&DelegateKvMgrCallback::Callback, &delegateKvMgrCallback, _1, _2);

    // use appid and userid to initialize a kvStoreDelegateManager, and set the default cfg.
    if (outManager != nullptr) {
        delete outManager;
        outManager = nullptr;
    }
    KvStoreDelegateManager *manager = new (std::nothrow) KvStoreDelegateManager(param.appId, param.userId);
    if (manager == nullptr) {
        MST_LOG("new delegate failed nullptr.");
        return nullptr;
    }
    DBStatus status = manager->SetKvStoreConfig(KV_CONFIG);
    if (status != DBStatus::OK) {
        MST_LOG("%s SetConfig failed! Status= %d", TAG.c_str(), status);
        delete manager;
        manager = nullptr;
        return nullptr;
    }

    KvStoreDelegate::Option option = TransferKvOptionType(optionParam);
    // get kv store, then the Callback will save the status and delegate.
    manager->GetKvStore(param.storeId, option, callFunction);
    status = delegateKvMgrCallback.GetStatus();
    if (status != DBStatus::OK) {
        MST_LOG("%s GetKvStore failed! Status= %d", TAG.c_str(), status);
        delete manager;
        manager = nullptr;
        return nullptr;
    }
    const KvStoreDelegate* delegate = const_cast<KvStoreDelegate*>(delegateKvMgrCallback.GetKvStore());
    if (delegate == nullptr) {
        MST_LOG("%s GetKvStore failed! delegate nullptr.", TAG.c_str());
        delete manager;
        manager = nullptr;
        return nullptr;
    }

    MST_LOG("%s GetKvStore success: %s %s %s %d %d", TAG.c_str(),
        param.storeId.c_str(), param.appId.c_str(), param.userId.c_str(), option.createIfNecessary, option.localOnly);
    outManager = manager;
    return const_cast<KvStoreDelegate*>(delegate);
}

KvStoreDelegate* DistributedTestTools::GetDelegateStatus(KvStoreDelegateManager *&outManager, DBStatus &statusReturn,
    const KvDBParameters &param, const KvOption &optionParam)
{
    MST_LOG("Delegate isEncryptedDb : %s", (optionParam.isEncryptedDb ? "true" : "false"));
    SetDir(DIRECTOR);
    if (param.storeId.empty() || param.appId.empty() || param.userId.empty()) {
        return nullptr;
    }

    // define a Callback to hold the kvStoreDelegate and status.
    DelegateKvMgrCallback delegateKvMgrCallback;
    function<void(DBStatus, KvStoreDelegate*)> functionCallback
        = bind(&DelegateKvMgrCallback::Callback, &delegateKvMgrCallback, _1, _2);

    // use appid and userid to initialize a kvStoreDelegateManager, and set the default cfg.
    if (outManager != nullptr) {
        delete outManager;
        outManager = nullptr;
    }
    KvStoreDelegateManager *manager = new (std::nothrow) KvStoreDelegateManager(param.appId, param.userId);
    if (manager == nullptr) {
        MST_LOG("new delegate failed nullptr.");
        return nullptr;
    }
    DBStatus status = manager->SetKvStoreConfig(KV_CONFIG);
    if (status != DBStatus::OK) {
        MST_LOG("%s SetConfig failed! Status= %d", TAG.c_str(), status);
        delete manager;
        manager = nullptr;
        return nullptr;
    }

    KvStoreDelegate::Option option = TransferKvOptionType(optionParam);
    // get kv store, then the Callback will save the status and delegate.
    manager->GetKvStore(param.storeId, option, functionCallback);
    statusReturn = delegateKvMgrCallback.GetStatus();
    if (statusReturn != DBStatus::OK) {
        MST_LOG("%s GetKvStore failed! Status= %d", TAG.c_str(), statusReturn);
        delete manager;
        manager = nullptr;
        return nullptr;
    }
    const KvStoreDelegate* delegate = const_cast<KvStoreDelegate*>(delegateKvMgrCallback.GetKvStore());
    if (delegate == nullptr) {
        MST_LOG("%s GetDelegate failed! delegate is nullptr.", TAG.c_str());
        delete manager;
        manager = nullptr;
        return nullptr;
    }

    MST_LOG("%s GetKvStore success : %s %s %s %d %d", TAG.c_str(), param.storeId.c_str(), param.appId.c_str(),
        param.userId.c_str(), option.createIfNecessary, option.localOnly);
    outManager = manager;
    return const_cast<KvStoreDelegate*>(delegate);
}

// This static method is for moduleTest of distributed to try-get KvStoreDelegate with invalid params.
DBStatus DistributedTestTools::GetDelegateNotGood(KvStoreDelegateManager *&manager, KvStoreDelegate *&outDelegate,
    const string &storeId, const string &appId, const string &userId, const KvOption &optionParam)
{
    SetDir(DIRECTOR);

    // define a Callback to hold the kvStoreDelegate and status.
    DelegateKvMgrCallback delegateKvMgrCallback;
    function<void(DBStatus, KvStoreDelegate*)> callFunction
        = bind(&DelegateKvMgrCallback::Callback, &delegateKvMgrCallback, _1, _2);

    // use appid and userid to initialize a kvStoreDelegateManager, and set the default cfg.
    manager = new (std::nothrow) KvStoreDelegateManager(appId, userId);
    if (manager == nullptr) {
        MST_LOG("new delegate failed nullptr.");
        return DBStatus::DB_ERROR;
    }
    DBStatus status = manager->SetKvStoreConfig(KV_CONFIG);
    if (status != DBStatus::OK) {
        MST_LOG("%s SetConfig failed! Status= %d", TAG.c_str(), status);
        status = DBStatus::INVALID_ARGS;
        delete manager;
        manager = nullptr;
        return status;
    }

    KvStoreDelegate::Option option = TransferKvOptionType(optionParam);
    // get kv store, then the Callback will save the status and delegate.
    manager->GetKvStore(storeId, option, callFunction);
    status = delegateKvMgrCallback.GetStatus();
    outDelegate = const_cast<KvStoreDelegate*>(delegateKvMgrCallback.GetKvStore());
    if (outDelegate != nullptr) {
        MST_LOG("%s GetKvStore failed! delegate nullptr.", TAG.c_str());
        delete manager;
        manager = nullptr;
        return status;
    }
    if (status == DBStatus::NOT_FOUND) {
        MST_LOG("%s GetKvStore failed! Status= %d", TAG.c_str(), status);
        delete manager;
        manager = nullptr;
        return status;
    }

    MST_LOG("%s GetKvStore failed, Status = %d", TAG.c_str(), status);
    return status;
}

// This static method is for moduleTest to put value with kvStoreDelegate.
DBStatus DistributedTestTools::Put(KvStoreDelegate &kvStoreDelegate, const Key &key, const Value &value)
{
    return kvStoreDelegate.Put(key, value);
}

// This static method is for moduleTest to putBatch value with kvStoreDelegate.
DBStatus DistributedTestTools::PutBatch(KvStoreDelegate &kvStoreDelegate, const vector<Entry> &entries)
{
    DistributedDB::DBStatus status;
    unsigned int cnt = entries.size();
    int index = 0;
    while (cnt > BATCH_RECORDS) {
        cnt -= BATCH_RECORDS;
        std::vector<DistributedDB::Entry> entriesBatch(entries.begin() + index * BATCH_RECORDS,
            entries.begin() + (index + 1) * BATCH_RECORDS);
        status = kvStoreDelegate.PutBatch(entriesBatch);
        index++;
        if (status != DBStatus::OK) {
            return status;
        }
    }
    std::vector<DistributedDB::Entry> entriesBatch(entries.begin() + index * BATCH_RECORDS, entries.end());
    status = kvStoreDelegate.PutBatch(entriesBatch);

    return status;
}

// This static method is for moduleTest to delete value with kvStoreDelegate.
DBStatus DistributedTestTools::Delete(KvStoreDelegate &kvStoreDelegate, const Key &key)
{
    return kvStoreDelegate.Delete(key);
}

// This static method is for moduleTest to deleteBatch value with kvStoreDelegate.
DBStatus DistributedTestTools::DeleteBatch(KvStoreDelegate &kvStoreDelegate, const vector<Key> &keys)
{
    if (keys.size() > BATCH_RECORDS) {
        int cnt = 0;
        std::vector<DistributedDB::Key> keyBatch;
        DistributedDB::DBStatus status;
        for (const auto &iter : keys) {
            keyBatch.push_back(iter);
            cnt++;
            if (cnt % BATCH_RECORDS == 0 || cnt == static_cast<int>(keys.size())) {
                status = kvStoreDelegate.DeleteBatch(keyBatch);
                if (status != DBStatus::OK) {
                    return status;
                }
                keyBatch.clear();
            }
        }
        return status;
    } else {
        return kvStoreDelegate.DeleteBatch(keys);
    }
}

// This static method is for moduleTest to clear value with kvStoreDelegate.
DBStatus DistributedTestTools::Clear(KvStoreDelegate &kvStoreDelegate)
{
    return kvStoreDelegate.Clear();
}

// This static method is for moduleTest to try-get snapshot with kvStoreDelegate.
KvStoreSnapshotDelegate *DistributedTestTools::GetKvStoreSnapshot(KvStoreDelegate &kvStoreDelegate)
{
    DelegateCallback delegateCallback;
    function<void(DBStatus, KvStoreSnapshotDelegate *)> callFunction
        = bind(&DelegateCallback::Callback, &delegateCallback, _1, _2);

    // no need to use obsever, so param1 is nullptr;
    kvStoreDelegate.GetKvStoreSnapshot(nullptr, callFunction);
    DBStatus status = delegateCallback.GetStatus();
    if (status != DBStatus::OK) {
        MST_LOG("%s Get Callback failed! Status= %d", TAG.c_str(), status);
        return nullptr;
    }
    KvStoreSnapshotDelegate *snapshot
        = const_cast<KvStoreSnapshotDelegate *>(delegateCallback.GetKvStoreSnapshot());
    if (snapshot == nullptr) {
        MST_LOG("%s Get KvStoreSnapshot null! Status= %d", TAG.c_str(), status);
        return nullptr;
    }
    return snapshot;
}

// This static method is for moduleTest to get value with kvStoreDelegate.
Value DistributedTestTools::Get(KvStoreDelegate &kvStoreDelegate, const Key &key)
{
    // initialize the result value.
    Value result = { };
    DelegateCallback delegateCallback;
    function<void(DBStatus, KvStoreSnapshotDelegate *)> callFunction
        = bind(&DelegateCallback::Callback, &delegateCallback, _1, _2);
    kvStoreDelegate.GetKvStoreSnapshot(nullptr, callFunction);
    DBStatus status = delegateCallback.GetStatus();
    if (status != DBStatus::OK) {
        MST_LOG("%s Get failed! Status= %d", TAG.c_str(), status);
        return result;
    }

    // the first Callback's kvStoreDelegate is used to get snapshot.
    KvStoreSnapshotDelegate *snapshot
        = const_cast<KvStoreSnapshotDelegate*>(delegateCallback.GetKvStoreSnapshot());
    if (snapshot == nullptr) {
        MST_LOG("%s Get snapshot null! Status= %d", TAG.c_str(), status);
        return result;
    }

    // the second Callback is used in Snapshot.
    KvCallback kvCallback;
    function < void(DBStatus, Value) > function2
        = bind(&KvCallback::Callback, &kvCallback, _1, _2);
    snapshot->Get(key, function2);
    status = kvCallback.GetStatus();
    if (status != DBStatus::OK) {
        kvStoreDelegate.ReleaseKvStoreSnapshot(snapshot);
        snapshot = nullptr;
        MST_LOG("%s Get value failed! Status= %d", TAG.c_str(), status);
        return result;
    }
    result = kvCallback.GetValue();
    kvStoreDelegate.ReleaseKvStoreSnapshot(snapshot);
    snapshot = nullptr;
    return result;
}

Value DistributedTestTools::Get(KvStoreSnapshotDelegate &kvStoreSnapshotDelegate, const Key &key)
{
    KvCallback kvCallback;
    function < void(DBStatus, Value) > function2
        = bind(&KvCallback::Callback, &kvCallback, _1, _2);
    kvStoreSnapshotDelegate.Get(key, function2);
    return kvCallback.GetValue();
}

vector<Entry> DistributedTestTools::GetEntries(KvStoreSnapshotDelegate &kvStoreSnapshotDelegate, const Key &key)
{
    KvCallback kvCallback;
    function<void(DBStatus, vector<Entry>)> function2
        = bind(&KvCallback::CallbackPrefix, &kvCallback, _1, _2);
    kvStoreSnapshotDelegate.GetEntries(key, function2);
    return kvCallback.GetValueVector();
}

// This static method is for moduleTest to get values with kvStoreDelegate and keyPrefix.
vector<Entry> DistributedTestTools::GetEntries(KvStoreDelegate &kvStoreDelegate, const Key &keyPrefix)
{
    DelegateCallback delegateCallback;
    function<void(DBStatus, KvStoreSnapshotDelegate *)> callFunction
        = bind(&DelegateCallback::Callback, &delegateCallback, _1, _2);
    kvStoreDelegate.GetKvStoreSnapshot(nullptr, callFunction);
    DBStatus status = delegateCallback.GetStatus();
    if (status != DBStatus::OK) {
        MST_LOG("%s Get failed! Status= %d", TAG.c_str(), status);
        return {};
    }

    KvStoreSnapshotDelegate *snapshot
        = const_cast<KvStoreSnapshotDelegate*>(delegateCallback.GetKvStoreSnapshot());
    if (snapshot == nullptr) {
        MST_LOG("%s Get snapshot null! Status= %d", TAG.c_str(), status);
        return {};
    }

    KvCallback kvCallback;
    function<void(DBStatus, vector<Entry>)> function2 = bind(&KvCallback::CallbackPrefix, &kvCallback, _1, _2);
    snapshot->GetEntries(keyPrefix, function2);
    status = kvCallback.GetStatus();
    if (status != DBStatus::OK) {
        kvStoreDelegate.ReleaseKvStoreSnapshot(snapshot);
        snapshot = nullptr;
        MST_LOG("%s GetEntries failed! Status= %d", TAG.c_str(), status);
        return {};
    }
    kvStoreDelegate.ReleaseKvStoreSnapshot(snapshot);
    snapshot = nullptr;
    return kvCallback.GetValueVector();
}

void TickTock(steady_clock::time_point &tick, double &duration)
{
    steady_clock::time_point tock = steady_clock::now();
    auto durate = duration_cast<microseconds>(tock - tick);
    tick = tock;
    duration = static_cast<double>(durate.count());
}

bool TickTock1(steady_clock::time_point &tick, int putGetTimes, double &duration)
{
    if (putGetTimes <= 0) {
        MST_LOG("putGetTimes is 0!");
        return false;
    }
    TickTock(tick, duration);
    duration /= putGetTimes;
    return true;
}

void GetSysInfo(DistributedTestSysInfo &si, SeqNo seqNo)
{
    si.GetSysMemOccpy(seqNo);
    si.GetSysCpuUsage(seqNo, DEFAULT_INTEVAL);
    si.GetSysCurrentPower(seqNo, DEFAULT_COUNT, DEFAULT_INTEVAL);
}

void CheckBeforeOpenDB(bool getSysInfo, steady_clock::time_point &tick,
    const vector<Entry> &data1, const vector<Key> &keys, DistributedTestSysInfo &si)
{
    if (!getSysInfo) {
        MST_LOG("[gen data]:%zu, %zu.", data1.size(), keys.size());
        tick = time_point_cast<microseconds>(steady_clock::now());
    } else {
        MST_LOG("System info before opening a db:");
        GetSysInfo(si, FIRST);
    }
}

void CheckAfterOperateDB(bool getSysInfo, steady_clock::time_point &tick,
    double &duration, DistributedTestSysInfo &si)
{
    if (!getSysInfo) {
        TickTock(tick, duration);
        MST_LOG("[time calculator]this operate cost %f us.", duration);
    } else {
        MST_LOG("System info after opening db, or before inserting records:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void PutDuration(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    if (!getSysInfo) {
        double duration;
        TickTock(tick, duration);
        performanceData.putDuration = duration / putGetTimes;
        MST_LOG("[time calculator]put first [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
            putGetTimes, keyLength, duration, performanceData.putDuration);
        TickTock(tick, duration);
    } else {
        MST_LOG("System info after inserting records, or before querying records:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void ReadDuration(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    double duration;
    if (!getSysInfo) {
        TickTock(tick, duration);
        performanceData.readPutDuration = duration / putGetTimes;
        MST_LOG("[time calculator]get first [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
            putGetTimes, keyLength, duration, performanceData.readPutDuration);
        TickTock(tick, duration);
    } else {
        MST_LOG("System info after querying records, or before updating records:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void UpdateDuration(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    double duration;
    if (!getSysInfo) {
        TickTock(tick, duration);
        performanceData.updateDuration = duration / putGetTimes;
        MST_LOG("[time calculator]put second [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
            putGetTimes, keyLength, duration, performanceData.updateDuration);
        TickTock(tick, duration);
    } else {
        MST_LOG("System info after updating records, or before querying records one by one:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void ReadUpdateDuration(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    double duration;
    if (!getSysInfo) {
        TickTock(tick, duration);
        performanceData.readUpdateDuration = duration / putGetTimes;
        MST_LOG("[time calculator]get second [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
            putGetTimes, keyLength, duration, performanceData.readUpdateDuration);
        TickTock(tick, duration);
    } else {
        MST_LOG("System info after querying records one by one, or before deleting records one by one:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void ClearDuration(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    double duration;
    if (!getSysInfo) {
        TickTock(tick, duration);
        performanceData.deleteDuration = duration / putGetTimes;
        MST_LOG("[time calculator]delete [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
            putGetTimes, keyLength, duration, performanceData.deleteDuration);
        TickTock(tick, duration);
    } else {
        MST_LOG("System info after deleting records one by one, or before closing a db:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

bool DistributedTestTools::CalculateOpenPerformance(PerformanceData &performanceData)
{
    int putGetTimes = performanceData.putGetTimes;
    int keyLength = performanceData.keyLength;
    int valueLength = performanceData.valueLength;
    bool getSysInfo = performanceData.getSysInfo;
    DistributedTestSysInfo si;
    steady_clock::time_point tick;

    vector<Entry> data1 = GenRanKeyVal(putGetTimes, keyLength, valueLength, 'a');
    vector<Key> keys = GetKeysFromEntries(data1, false);

    CheckBeforeOpenDB(getSysInfo, tick, data1, keys, si);
    // print the opened time.
    KvStoreDelegate *store1 = nullptr;
    KvStoreDelegateManager *manager = nullptr;

    KvOption option = g_kvOption;
    option.localOnly = performanceData.isLocal;
    store1 = DistributedTestTools::GetDelegateSuccess(manager, KVDB_PARAMETER_PERFORM, option);
    if (store1 == nullptr) {
        return false;
    }
    CheckAfterOperateDB(getSysInfo, tick, performanceData.closeDuration, si);

    // close the base db
    if (manager->CloseKvStore(store1) != DistributedDB::DBStatus::OK) {
        return false;
    }
    if (manager->DeleteKvStore(STORE_ID_PERFORM) != OK) {
        return false;
    }
    store1 = nullptr;
    delete manager;
    manager = nullptr;
    CheckAfterOperateDB(getSysInfo, tick, performanceData.closeDuration, si);

    return true;
}

bool DistributedTestTools::CalculateInsertPerformance(PerformanceData &performanceData)
{
    int putGetTimes = performanceData.putGetTimes;
    int keyLength = performanceData.keyLength;
    int valueLength = performanceData.valueLength;
    bool putBatch = performanceData.putBatch;
    bool getSysInfo = performanceData.getSysInfo;
    DistributedTestSysInfo si;
    steady_clock::time_point tick;

    vector<Entry> data1 = GenRanKeyVal(putGetTimes, keyLength, valueLength, 'a');
    vector<Key> keys = GetKeysFromEntries(data1, false);
    vector< vector<Entry> > groupEntries = GetGroupEntries(data1, BATCH_RECORDS, valueLength, 'a');
    CheckBeforeOpenDB(getSysInfo, tick, data1, keys, si);

    KvStoreDelegate *store1 = nullptr;
    KvStoreDelegateManager *manager = nullptr;

    KvOption option = g_kvOption;
    option.localOnly = performanceData.isLocal;
    store1 = DistributedTestTools::GetDelegateSuccess(manager, KVDB_PARAMETER_PERFORM, option);
    if (store1 == nullptr) {
        return false;
    }
    // print the insert time.
    if (putBatch) {
        for (auto entries = groupEntries.begin(); entries != groupEntries.end(); ++entries) {
            DistributedTestTools::PutBatch(*store1, *entries);
        }
    } else {
        for (auto entry = data1.begin(); entry != data1.end(); ++entry) {
            DistributedTestTools::Put(*store1, entry->key, entry->value);
        }
    }

    PutDuration(tick, performanceData, keyLength, si);

    // close the base db
    if (manager->CloseKvStore(store1) != DistributedDB::DBStatus::OK) {
        return false;
    }
    if (manager->DeleteKvStore(STORE_ID_PERFORM) != OK) {
        return false;
    }
    store1 = nullptr;
    delete manager;
    manager = nullptr;
    CheckAfterOperateDB(getSysInfo, tick, performanceData.closeDuration, si);

    return true;
}

bool DistributedTestTools::CalculateGetPutPerformance(PerformanceData &performanceData)
{
    int putGetTimes = performanceData.putGetTimes;
    int keyLength = performanceData.keyLength;
    int valueLength = performanceData.valueLength;
    bool getBatch = performanceData.getBatch;
    bool getSysInfo = performanceData.getSysInfo;
    DistributedTestSysInfo si;
    steady_clock::time_point tick;

    vector<Entry> data1 = GenRanKeyVal(putGetTimes, keyLength, valueLength, 'a');
    vector<Key> keys = GetKeysFromEntries(data1, false);

    CheckBeforeOpenDB(getSysInfo, tick, data1, keys, si);

    // print the get put time.
    KvStoreDelegate *store1 = nullptr;
    KvStoreDelegateManager *manager = nullptr;

    KvOption option = g_kvOption;
    option.localOnly = performanceData.isLocal;
    store1 = DistributedTestTools::GetDelegateSuccess(manager, KVDB_PARAMETER_PERFORM, option);
    if (store1 == nullptr) {
        return false;
    }
    KvStoreSnapshotDelegate *snapShot = DistributedTestTools::GetKvStoreSnapshot(*store1);
    if (snapShot == nullptr)  {
        return false;
    }
    if (getBatch) {
        DistributedTestTools::GetEntries(*snapShot, PERFORMANCEKEY);
    } else {
        for (auto key = keys.begin(); key != keys.end(); ++key) {
            DistributedTestTools::Get(*snapShot, *key);
        }
    }
    store1->ReleaseKvStoreSnapshot(snapShot);
    snapShot = nullptr;
    ReadDuration(tick, performanceData, keyLength, si);

    // close the base db
    if (manager->CloseKvStore(store1) != DistributedDB::DBStatus::OK) {
        return false;
    }
    if (manager->DeleteKvStore(STORE_ID_PERFORM) != OK) {
        return false;
    }
    store1 = nullptr;
    delete manager;
    manager = nullptr;
    CheckAfterOperateDB(getSysInfo, tick, performanceData.closeDuration, si);

    return true;
}

bool DistributedTestTools::CalculateUpdatePerformance(PerformanceData &performanceData)
{
    int putGetTimes = performanceData.putGetTimes;
    int keyLength = performanceData.keyLength;
    int valueLength = performanceData.valueLength;
    bool putBatch = performanceData.putBatch;
    bool getSysInfo = performanceData.getSysInfo;
    DistributedTestSysInfo si;
    steady_clock::time_point tick;

    vector<Entry> data1 = GenRanKeyVal(putGetTimes, keyLength, valueLength, 'a');
    vector<Key> keys = GetKeysFromEntries(data1, false);
    Value updateVal;
    updateVal.assign(valueLength, 'b');
    vector< vector<Entry> > groupEntriesUp = GetGroupEntries(data1, BATCH_RECORDS, valueLength, 'b');
    CheckBeforeOpenDB(getSysInfo, tick, data1, keys, si);

    // print the update time.
    KvStoreDelegate *store1 = nullptr;
    KvStoreDelegateManager *manager = nullptr;

    KvOption option = g_kvOption;
    option.localOnly = performanceData.isLocal;
    store1 = DistributedTestTools::GetDelegateSuccess(manager, KVDB_PARAMETER_PERFORM, option);
    if (store1 == nullptr) {
        return false;
    }
    if (putBatch) {
        for (auto entries = groupEntriesUp.begin(); entries != groupEntriesUp.end(); ++entries) {
            DistributedTestTools::PutBatch(*store1, *entries);
        }
    } else {
        for (auto entry = data1.begin(); entry != data1.end(); ++entry) {
            DistributedTestTools::Put(*store1, entry->key, updateVal);
        }
    }
    UpdateDuration(tick, performanceData, keyLength, si);

    // close the base db
    if (manager->CloseKvStore(store1) != DistributedDB::DBStatus::OK) {
        return false;
    }
    if (manager->DeleteKvStore(STORE_ID_PERFORM) != OK) {
        return false;
    }
    store1 = nullptr;
    delete manager;
    manager = nullptr;
    CheckAfterOperateDB(getSysInfo, tick, performanceData.closeDuration, si);

    return true;
}

bool DistributedTestTools::CalculateGetUpdatePerformance(PerformanceData &performanceData)
{
    int putGetTimes = performanceData.putGetTimes;
    int keyLength = performanceData.keyLength;
    int valueLength = performanceData.valueLength;
    bool getSysInfo = performanceData.getSysInfo;
    DistributedTestSysInfo si;
    steady_clock::time_point tick;

    vector<Entry> data1 = GenRanKeyVal(putGetTimes, keyLength, valueLength, 'a');
    vector<Key> keys = GetKeysFromEntries(data1, false);

    CheckBeforeOpenDB(getSysInfo, tick, data1, keys, si);

    // print the get update time.
    KvStoreDelegate *store1 = nullptr;
    KvStoreDelegateManager *manager = nullptr;

    KvOption option = g_kvOption;
    option.localOnly = performanceData.isLocal;
    store1 = DistributedTestTools::GetDelegateSuccess(manager, KVDB_PARAMETER_PERFORM, option);
    if (store1 == nullptr) {
        return false;
    }
    KvStoreSnapshotDelegate *snapShot = DistributedTestTools::GetKvStoreSnapshot(*store1);
    if (snapShot == nullptr)  {
        return false;
    }
    for (auto key = keys.begin(); key != keys.end(); ++key) {
        Value valueResult = DistributedTestTools::Get(*snapShot, *key);
    }
    store1->ReleaseKvStoreSnapshot(snapShot);
    snapShot = nullptr;
    ReadUpdateDuration(tick, performanceData, keyLength, si);

    // close the base db
    if (manager->CloseKvStore(store1) != DistributedDB::DBStatus::OK) {
        return false;
    }
    if (manager->DeleteKvStore(STORE_ID_PERFORM) != OK) {
        return false;
    }
    store1 = nullptr;
    delete manager;
    manager = nullptr;
    CheckAfterOperateDB(getSysInfo, tick, performanceData.closeDuration, si);
    return true;
}

bool DistributedTestTools::CalculateUseClearPerformance(PerformanceData &performanceData)
{
    int putGetTimes = performanceData.putGetTimes;
    int keyLength = performanceData.keyLength;
    int valueLength = performanceData.valueLength;
    bool useClear = performanceData.useClear;
    bool getSysInfo = performanceData.getSysInfo;
    DistributedTestSysInfo si;
    steady_clock::time_point tick;

    vector<Entry> data1 = GenRanKeyVal(putGetTimes, keyLength, valueLength, 'a');
    vector<Key> keys = GetKeysFromEntries(data1, false);

    CheckBeforeOpenDB(getSysInfo, tick, data1, keys, si);

    // print the get update time.
    KvStoreDelegate *store1 = nullptr;
    KvStoreDelegateManager *manager = nullptr;

    KvOption option = g_kvOption;
    option.localOnly = performanceData.isLocal;
    store1 = DistributedTestTools::GetDelegateSuccess(manager, KVDB_PARAMETER_PERFORM, option);
    if (store1 == nullptr) {
        return false;
    }
    if (useClear) {
        DistributedTestTools::Clear(*store1);
    } else {
        for (auto key = keys.begin(); key != keys.end(); ++key) {
            DistributedTestTools::Delete(*store1, *key);
        }
    }
    ClearDuration(tick, performanceData, keyLength, si);

    // close the base db
    if (manager->CloseKvStore(store1) != DistributedDB::DBStatus::OK) {
        return false;
    }
    if (manager->DeleteKvStore(STORE_ID_PERFORM) != OK) {
        return false;
    }
    store1 = nullptr;
    delete manager;
    manager = nullptr;
    CheckAfterOperateDB(getSysInfo, tick, performanceData.closeDuration, si);
    return true;
}

bool DistributedTestTools::CloseAndRelease(KvStoreDelegateManager *&manager, KvStoreDelegate *&delegate)
{
    bool result = true;
    DBStatus status;
    if (delegate != nullptr && manager != nullptr) {
        status = manager->CloseKvStore(delegate);
        MST_LOG("[CloseAndRelease] status = %d", status);
        result = (status == OK);
        delegate = nullptr;
        delete manager;
        manager = nullptr;
    } else {
        MST_LOG("Close Failed");
        return false;
    }
    return result;
}

bool DistributedTestTools::CloseAndRelease(KvStoreDelegateManager *&manager, KvStoreNbDelegate *&delegate)
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

bool DistributedTestTools::VerifyDbRecordCnt(KvStoreNbDelegate *&delegate, unsigned int recordCnt, bool isLocal)
{
    if (delegate == nullptr) {
        MST_LOG("The delegate is nullptr!");
        return false;
    }
    DBStatus status = OK;
    vector<Entry> entries;
    if (isLocal) {
#ifdef RELEASE_MODE_V2
        status = delegate->GetLocalEntries(KEY_EMPTY, entries);
#endif // end of RELEASE_MODE_V2
    } else {
        status = delegate->GetEntries(KEY_EMPTY, entries);
    }

    if (status == OK && entries.size() == recordCnt) {
        return true;
    } else {
        MST_LOG("The real status is %d, real recordCnt is %zu", status, entries.size());
        return false;
    }
}

bool DistributedTestTools::VerifyRecordsInDb(DistributedDB::KvStoreNbDelegate *&delegate,
    std::vector<DistributedDB::Entry> &entriesExpected, const std::vector<uint8_t> &keyPrefix, bool isLocal)
{
    DBStatus status = OK;
    vector<Entry> entries;
    if (isLocal) {
#ifdef RELEASE_MODE_V2
        status = delegate->GetLocalEntries(keyPrefix, entries);
#endif // end of RELEASE_MODE_V2
    } else {
        status = delegate->GetEntries(keyPrefix, entries);
    }
    if (status == OK && CompareEntriesVector(entries, entriesExpected)) {
        return true;
    }
    MST_LOG("[VerifyRecordsInDb] status = %d, entries.size() = %zu\n", status, entries.size());
    return false;
}

void ReleaseSqliteResource(sqlite3_stmt *&statement, sqlite3 *&db)
{
    sqlite3_finalize(statement);
    statement = nullptr;
    sqlite3_close(db);
}

bool SqliteBindToStatement(sqlite3_stmt *&statement, sqlite3 *&db, std::vector<DistributedDB::Key> &sqlParam)
{
    int errCode;
    for (unsigned int index = 0; index < sqlParam.size(); index++) {
        if (sqlParam[index].empty()) {
            errCode = sqlite3_bind_zeroblob(statement, index, -1); // -1 for zero-length blob.
        } else {
            errCode = sqlite3_bind_blob(statement, index + 1, static_cast<const void*>(sqlParam[index].data()),
                sqlParam[index].size(), SQLITE_TRANSIENT);
        }
        if (errCode != SQLITE_OK) {
            MST_LOG("[SqliteBindToStatement] Failed to bind the SQLite blob: %d", errCode);
            ReleaseSqliteResource(statement, db);
            return false;
        }
    }
    return true;
}

static bool SetSQLiteKey(sqlite3 *db, const std::vector<uint8_t> &passwd)
{
    int errCode = sqlite3_key(db, static_cast<const void *>(passwd.data()),
                              static_cast<int>(passwd.size()));
    if (errCode != SQLITE_OK) {
        MST_LOG("[DistributedTestTools::SetSQLiteKey] sqlite3_key failed: %d", errCode);
        return false;
    }
    char *errMsg = nullptr;
    bool result = false;
    errCode = sqlite3_exec(db, CIPHER_CONFIG_SQL.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK) {
        MST_LOG("[DistributedTestTools::SetSQLiteKey] config cipher failed: %d", errCode);
        goto END;
    }

    errCode = sqlite3_exec(db, KDF_ITER_CONFIG_SQL.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK) {
        MST_LOG("[DistributedTestTools::SetSQLiteKey] config kdf iter failed: %d", errCode);
        goto END;
    }
    result = true;
END:
    if (errMsg != nullptr) {
        sqlite3_free(errMsg);
        errMsg = nullptr;
    }
    return result;
}

bool DistributedTestTools::GetRecordCntByKey(const std::string &dbName,
    const std::string &strSql, std::vector<DistributedDB::Key> &sqlParam, KvOption &option, int &count)
{
    sqlite3 *db = nullptr;
    std::string dbOpenName = dbName;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE;
    int errCode = sqlite3_open_v2(dbOpenName.c_str(), &db, flag, nullptr);
    if (errCode != SQLITE_OK) {
        MST_LOG("[DistributedTestTools::GetRecordCntByKey] sqlite3_open failed: %d", errCode);
        return false;
    }

    if (option.isEncryptedDb) {
        if (SetSQLiteKey(db, option.passwd) != true) {
            MST_LOG("[DistributedTestTools::GetRecordCntByKey] sqlite3 set key failed");
            sqlite3_close(db);
            return false;
        }
    }

    sqlite3_stmt *statement = nullptr;
    errCode = sqlite3_prepare(db, strSql.c_str(), -1, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        MST_LOG("[DistributedTestTools::GetRecordCntByKey] sqlite3_prepare failed: %d", errCode);
        ReleaseSqliteResource(statement, db);
        return false;
    }
    if (!SqliteBindToStatement(statement, db, sqlParam)) {
        return false;
    }
    errCode = sqlite3_step(statement);
    if (errCode == SQLITE_ROW) {
        count = static_cast<uint64_t>(sqlite3_column_int64(statement, 0));
    } else {
        MST_LOG("[DistributedTestTools::GetRecordCntByKey] sqlite3_step failed: %d", errCode);
        ReleaseSqliteResource(statement, db);
        count = 0;
        return false;
    }

    ReleaseSqliteResource(statement, db);
    return true;
}

bool DistributedTestTools::QuerySpecifiedData(const std::string &dbName,
    const std::string &strSql, EncrypteAttribute &attribute, int &count)
{
    sqlite3 *db = nullptr;
    std::string dbOpenName = dbName;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE;
    int nResult = sqlite3_open_v2(dbOpenName.c_str(), &db, flag, nullptr);
    if (nResult != SQLITE_OK) {
        MST_LOG("[sqlite3_open] failed: %d.", nResult);
        return false;
    }

    if (attribute.isEncryptedDb) {
        if (SetSQLiteKey(db, attribute.passwd) != true) {
            MST_LOG("[DistributedTestTools::QuerySpecifiedData] sqlite3 set key failed");
            sqlite3_close(db);
            return false;
        }
    }

    sqlite3_stmt *statement = nullptr;
    nResult = sqlite3_prepare(db, strSql.c_str(), -1, &statement, nullptr);
    if (nResult != SQLITE_OK) {
        MST_LOG("[sqlite3_prepare] failed %d.", nResult);
        sqlite3_finalize(statement);
        statement = nullptr;
        sqlite3_close(db);
        return false;
    }
    int errCode = sqlite3_step(statement);
    if (errCode != SQLITE_ROW) {
        MST_LOG("[sqlite3_step] failed, errCode: %d.", errCode);
        sqlite3_finalize(statement);
        statement = nullptr;
        sqlite3_close(db);
        return false;
    }
    count = static_cast<uint64_t>(sqlite3_column_int64(statement, 0));
    MST_LOG("The query count is %d\n.", count);
    sqlite3_finalize(statement);
    statement = nullptr;
    sqlite3_close(db);
    return true;
}

bool DistributedTestTools::RepeatCheckAsyncResult(const std::function<bool(void)> &inPred, int repeatLimit,
    uint32_t repeatInterval)
{
    int limit = repeatLimit;
    while (limit != 0) {
        if (inPred()) {
            return true;
        }
        if (--limit <= 0) {
            MST_LOG("BREAK RepeatCheckAsyncResult :: limit %d\n", limit);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(repeatInterval));
    }
    return false;
}

bool DistributedTestTools::CompareKey(const Entry &entry1, const Entry &entry2)
{
    return entry1.key < entry2.key;
}
void DistributedTestTools::CopyFile(const string &srcFile, const string &destFile)
{
    int len = 0;
    char buff[BUF_LEN] = { 0 };

    FILE *pIn = fopen(srcFile.c_str(), "r");
    if (pIn == nullptr) {
        MST_LOG("Open srcFile failed!");
        return;
    }
    FILE *pOut = fopen(destFile.c_str(), "w");
    if (pOut == nullptr) {
        MST_LOG("Open destFile failed!");
        fclose(pIn);
        return;
    }

    while ((len = fread(buff, sizeof(char), sizeof(buff), pIn)) > 0) {
        fwrite(buff, sizeof(char), len, pOut);
    }
    fclose(pOut);
    fclose(pIn);
}

void TransactionBeforOpenDB(bool getSysInfo, steady_clock::time_point &tick,
    const vector<Entry> &data1, const vector<Key> &keys, DistributedTestSysInfo &si)
{
    if (!getSysInfo) {
        MST_LOG("[gen data]:%zu, %zu.", data1.size(), keys.size());
        tick = time_point_cast<microseconds>(steady_clock::now());
    } else {
        MST_LOG("System info before opening a db:");
        GetSysInfo(si, FIRST);
    }
}

void TransactionAfterOpenDB(bool getSysInfo, steady_clock::time_point &tick,
    PerformanceData &performanceData, DistributedTestSysInfo &si)
{
    if (!getSysInfo) {
        TickTock(tick, performanceData.openDuration);
        MST_LOG("[time calculator]open a new db cost %f us.", static_cast<double>(performanceData.openDuration));
    } else {
        MST_LOG("System info after opening db, or before inserting records of a transaction:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void TransactionAfterPutData(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    if (!getSysInfo) {
        if (!TickTock1(tick, putGetTimes, performanceData.putDuration)) {
            MST_LOG("[time calculator]put first [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
                putGetTimes, keyLength, (performanceData.putDuration) * putGetTimes,
                performanceData.putDuration);
        }
    } else {
        MST_LOG("System info after inserting records of a transaction, or before querying records one by one:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void TransactionAfterQueryPutData(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    if (!getSysInfo) {
        if (!TickTock1(tick, putGetTimes, performanceData.readPutDuration)) {
            MST_LOG("[time calculator]get first [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
                putGetTimes, keyLength, (performanceData.readPutDuration) * putGetTimes,
                performanceData.readPutDuration);
        }
    } else {
        MST_LOG("System info after querying records one by one, or before updating records in a transaction:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void TransactionAfterUpdateData(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    if (!getSysInfo) {
        if (!TickTock1(tick, putGetTimes, performanceData.updateDuration)) {
            MST_LOG("[time calculator]put second [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
                putGetTimes, keyLength, (performanceData.updateDuration) * putGetTimes,
                performanceData.updateDuration);
        }
    } else {
        MST_LOG("System info after updating records in a transaction, or before updating records one by one:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void TransactionAfterQueryUpdateData(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    if (!getSysInfo) {
        if (!TickTock1(tick, putGetTimes, performanceData.readUpdateDuration)) {
            MST_LOG("[time calculator]get second [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
                putGetTimes, keyLength, (performanceData.readUpdateDuration) * putGetTimes,
                performanceData.readUpdateDuration);
        }
    } else {
        MST_LOG("System info after updating records one by one, or before deleting records in a transaction:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void TransactionAfterDeleteData(steady_clock::time_point &tick, PerformanceData &performanceData,
    int keyLength, DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    if (!getSysInfo) {
        if (!TickTock1(tick, putGetTimes, performanceData.deleteDuration)) {
            MST_LOG("[time calculator]delete [%d]keys,\tvalue[%dB-length],\tcost[%fus],\tper[%fus].",
                putGetTimes, keyLength, (performanceData.deleteDuration) * putGetTimes,
                performanceData.deleteDuration);
        }
    } else {
        MST_LOG("System info after deleting records in a transaction, or before closing a db:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

void TransactionAfterCloseDB(steady_clock::time_point &tick, PerformanceData &performanceData,
    DistributedTestSysInfo &si)
{
    bool getSysInfo = performanceData.getSysInfo;
    int putGetTimes = performanceData.putGetTimes;
    if (!getSysInfo) {
        if (!TickTock1(tick, putGetTimes, performanceData.closeDuration)) {
            MST_LOG("[time calculator]close a db cost %f us.", static_cast<double>(performanceData.closeDuration));
        }
    } else {
        MST_LOG("System info after closing a db:");
        GetSysInfo(si, SECOND);
        si.SaveSecondToFirst();
    }
}

bool BeginTransaction1(vector<Entry> &data1, KvStoreDelegate *store1)
{
    bool result = true;
    result = result && (store1->StartTransaction() == DBStatus::OK);
    for (auto entry = data1.begin(); entry != data1.end(); ++entry) {
        result = result && (DistributedTestTools::Put(*store1, entry->key, entry->value) == DBStatus::OK);
    }
    result = result && (store1->Commit() == DBStatus::OK);
    return result;
}

bool BeginTransaction2(vector<Entry> &data1, KvStoreDelegate *store1, Value updateVal)
{
    bool result = true;
    result = result && (store1->StartTransaction() == DBStatus::OK);
    for (auto entry = data1.begin(); entry != data1.end(); ++entry) {
        result = result && (DistributedTestTools::Put(*store1, entry->key, updateVal) == DBStatus::OK);
    }
    result = result && (store1->Commit() == DBStatus::OK);
    return result;
}

bool BeginTransaction3(KvStoreDelegate *store1, vector<Key> &keys)
{
    bool result = true;
    result = result && (store1->StartTransaction() == DBStatus::OK);
    for (auto key = keys.begin(); key != keys.end(); ++key) {
        result = result && (DistributedTestTools::Delete(*store1, *key) == DBStatus::OK);
    }
    result = result && (store1->Commit() == DBStatus::OK);
    return result;
}

bool CheckSnapShot1(KvStoreSnapshotDelegate *snapShot, KvStoreDelegate *store1, vector<Key> &keys)
{
    if (snapShot == nullptr) {
        return false;
    }
    for (auto key = keys.begin(); key != keys.end(); ++key) {
        Value valueResult = DistributedTestTools::Get(*snapShot, *key);
    }
    store1->ReleaseKvStoreSnapshot(snapShot);
    snapShot = nullptr;
    return true;
}

bool CheckSnapShot2(KvStoreSnapshotDelegate *snapShot, KvStoreDelegate *store1, vector<Key> &keys)
{
    snapShot = DistributedTestTools::GetKvStoreSnapshot(*store1);
    if (snapShot == nullptr)  {
        return false;
    }
    for (auto key = keys.begin(); key != keys.end(); ++key) {
        Value valueResult = DistributedTestTools::Get(*snapShot, *key);
    }
    store1->ReleaseKvStoreSnapshot(snapShot);
    snapShot = nullptr;
    return true;
}

bool CloseDB(KvStoreDelegateManager *manager, KvStoreDelegate *store1)
{
    DBStatus status = manager->CloseKvStore(store1);
    if (status != DistributedDB::DBStatus::OK) {
        return false;
    }
    store1 = nullptr;
    status = manager->DeleteKvStore(STORE_ID_PERFORM);
    if (status != DistributedDB::DBStatus::OK) {
        return false;
    }
    delete manager;
    manager = nullptr;
    return true;
}

bool DistributedTestTools::CalculateTransactionPerformance(PerformanceData &performanceData)
{
    int putGetTimes = performanceData.putGetTimes;
    int keyLength = performanceData.keyLength;
    int valueLength = performanceData.valueLength;
    bool getSysInfo = performanceData.getSysInfo;
    DistributedTestSysInfo si;
    steady_clock::time_point tick;
    vector<Entry> data1 = GenRanKeyVal(putGetTimes, keyLength, valueLength, 'a');
    vector<Key> keys = GetKeysFromEntries(data1, false);
    Value updateVal;
    updateVal.assign(valueLength, 'b');
    TransactionBeforOpenDB(getSysInfo, tick, data1, keys, si);
    // print the opened time.
    KvStoreDelegate *store1 = nullptr;
    KvStoreDelegateManager *manager = nullptr;
    KvOption option = g_kvOption;
    option.localOnly = performanceData.isLocal;
    store1 = GetDelegateSuccess(manager, KVDB_PARAMETER_PERFORM, option);
    if (store1 == nullptr) {
        return false;
    }
    TransactionAfterOpenDB(getSysInfo, tick, performanceData, si);
    // print the insert time.
    BeginTransaction1(data1, store1);
    TransactionAfterPutData(tick, performanceData, keyLength, si);
    // print the get time.
    KvStoreSnapshotDelegate *snapShot;
    snapShot = DistributedTestTools::GetKvStoreSnapshot(*store1);
    if (!(CheckSnapShot1(snapShot, store1, keys))) {
        return false;
    }
    TransactionAfterQueryPutData(tick, performanceData, keyLength, si);
    // print the update time.
    BeginTransaction2(data1, store1, updateVal);
    TransactionAfterUpdateData(tick, performanceData, keyLength, si);
    // print the get time.
    if (!(CheckSnapShot2(snapShot, store1, keys))) {
        return false;
    }
    TransactionAfterQueryUpdateData(tick, performanceData, keyLength, si);
    BeginTransaction3(store1, keys);
    TransactionAfterDeleteData(tick, performanceData, keyLength, si);
    // close the base db
    if (!(CloseDB(manager, store1))) {
        return false;
    }
    TransactionAfterCloseDB(tick, performanceData, si);
    return true;
}

KvStoreSnapshotDelegate *DistributedTestTools::RegisterSnapObserver(KvStoreDelegate *delegate,
    KvStoreObserver *observer)
{
    DelegateCallback delegateCallback;
    function<void(DBStatus, KvStoreSnapshotDelegate *)> callFunction
        = bind(&DelegateCallback::Callback, &delegateCallback, _1, _2);

    delegate->GetKvStoreSnapshot(observer, callFunction);
    DBStatus status = delegateCallback.GetStatus();
    if (status != DBStatus::OK) {
        MST_LOG("%s Get failed! Status= %d", TAG.c_str(), status);
        return nullptr;
    }

    KvStoreSnapshotDelegate *snapshot
        = const_cast<KvStoreSnapshotDelegate *>(delegateCallback.GetKvStoreSnapshot());
    if (snapshot == nullptr) {
        MST_LOG("%s Get snapshot null! Status= %d", TAG.c_str(), status);
        return nullptr;
    }
    return snapshot;
}

DBStatus DistributedTestTools::RegisterObserver(KvStoreDelegate *delegate, KvStoreObserver *observer)
{
    return delegate->RegisterObserver(observer);
}

DBStatus DistributedTestTools::UnRegisterObserver(KvStoreDelegate *delegate, KvStoreObserver *observer)
{
    return delegate->UnRegisterObserver(observer);
}

KvStoreDelegate::Option DistributedTestTools::TransferKvOptionType(const KvOption &optionParam)
{
    KvStoreDelegate::Option option;
    option.createIfNecessary = optionParam.createIfNecessary;
    option.localOnly = optionParam.localOnly;
    option.isEncryptedDb = optionParam.isEncryptedDb;
    option.cipher = optionParam.cipher;
    (void)option.passwd.SetValue(optionParam.passwd.data(), optionParam.passwd.size());
    return option;
}

std::string TransferStringToHashHexString(const std::string &origStr)
{
    SHA256_CTX context;
    SHA256_Init(&context);
    SHA256_Update(&context, origStr.data(), origStr.size());
    std::vector<uint8_t> hashVect(SHA256_DIGEST_LENGTH, 0);
    SHA256_Final(hashVect.data(), &context);

    const char *hex = "0123456789abcdef";
    std::string tmp;
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        tmp.push_back(hex[hashVect[i] >> 4]); // high 4 bit to one hex.
        tmp.push_back(hex[hashVect[i] & 0x0F]); // low 4 bit to one hex.
    }
    return tmp;
}

#if defined(RUNNING_ON_LINUX)
int RemoveDatabaseDirectory(const std::string &directory)
{
    return remove(directory.c_str());
}
#else
int RemoveDatabaseDirectory(const std::string &directory)
{
    return rmdir(directory.c_str());
}
#endif

bool VerifyObserverResult(const KvStoreObserverImpl &pObserver,
    int changedTimes, ListType type, const list<Entry> &lst, uint32_t timeout)
{
    MST_LOG("[VerifyObserverResult] pObserver.GetCumulatedFlag() = %d, pObserver.GetChanged() = %d, type = %d",
        pObserver.GetCumulatedFlag(), pObserver.GetChanged(), type);
    if (pObserver.GetCumulatedFlag()) {
        int expectListSize = lst.size();
        pObserver.WaitUntilReachRecordCount(expectListSize, type, timeout);
    } else {
        pObserver.WaitUntilReachChangeCount(changedTimes, timeout);
    }

    list<Entry> retLst;
    if (pObserver.GetCumulatedFlag()) {
        if (type == INSERT_LIST) {
            MST_LOG("type == INSERT_LIST");
            retLst = pObserver.GetCumulatedInsertList();
        } else if (type == UPDATE_LIST) {
            MST_LOG("type == UPDATE_LIST");
            retLst = pObserver.GetCumulatedUpdateList();
        } else {
            MST_LOG("type == DELETE_LIST");
            retLst = pObserver.GetCumulatedDeleteList();
        }
    } else {
        if (type == INSERT_LIST) {
            MST_LOG("type == INSERT_LIST");
            retLst = pObserver.GetInsertList();
        } else if (type == UPDATE_LIST) {
            MST_LOG("type == UPDATE_LIST");
            retLst = pObserver.GetUpdateList();
        } else {
            MST_LOG("type == DELETE_LIST");
            retLst = pObserver.GetDeleteList();
        }
    }

    bool result = true;
    vector<Entry> retVec(retLst.begin(), retLst.end());
    vector<Entry> expectVec(lst.begin(), lst.end());
    result = CompareEntriesVector(retVec, expectVec);
    MST_LOG("VerifyObserverResult CompareEntriesVector result is %d", result);
    return result;
}

bool VerifyObserverResult(const KvStoreObserverImpl &pObserver, int changedTimes, ListType type,
    const vector<Entry> &vec, uint32_t timeout)
{
    list<Entry> entriesList(vec.begin(), vec.end());
    return VerifyObserverResult(pObserver, changedTimes, type, entriesList, timeout);
}

bool VerifyObserverForSchema(const KvStoreObserverImpl &pObserver,
    int changedTimes, ListType type, const vector<Entry> &expectEntry, uint32_t timeout)
{
    MST_LOG("[VerifyObserverForSchema] pObserver.GetCumulatedFlag() = %d, pObserver.GetChanged() = %d, type = %d",
        pObserver.GetCumulatedFlag(), pObserver.GetChanged(), type);
    if (pObserver.GetCumulatedFlag()) {
        int expectListSize = expectEntry.size();
        pObserver.WaitUntilReachRecordCount(expectListSize, type, timeout);
    } else {
        pObserver.WaitUntilReachChangeCount(changedTimes, timeout);
    }

    list<Entry> retLst;
    if (pObserver.GetCumulatedFlag()) {
        if (type == INSERT_LIST) {
            MST_LOG("type == INSERT_LIST");
            retLst = pObserver.GetCumulatedInsertList();
        } else if (type == UPDATE_LIST) {
            MST_LOG("type == UPDATE_LIST");
            retLst = pObserver.GetCumulatedUpdateList();
        } else {
            MST_LOG("type == DELETE_LIST");
            retLst = pObserver.GetCumulatedDeleteList();
        }
    } else {
        if (type == INSERT_LIST) {
            MST_LOG("type == INSERT_LIST");
            retLst = pObserver.GetInsertList();
        } else if (type == UPDATE_LIST) {
            MST_LOG("type == UPDATE_LIST");
            retLst = pObserver.GetUpdateList();
        } else {
            MST_LOG("type == DELETE_LIST");
            retLst = pObserver.GetDeleteList();
        }
    }

    vector<Entry> retVec(retLst.begin(), retLst.end());
    if (retVec.size() != expectEntry.size()) {
        MST_LOG("[VerifyObserverForSchema] the expect value size is not equal to the retVec size and return");
        return false;
    }

    for (uint64_t index = 0; index < expectEntry.size(); index++) {
        bool result = ((retVec[index].key == expectEntry[index].key) &&
            (retVec[index].value == expectEntry[index].value));
        if (!result) {
            MST_LOG("[VerifyObserverForSchema] compare list failed at the position: %llu", ULL(index));
            return false;
        }
    }
    return true;
}