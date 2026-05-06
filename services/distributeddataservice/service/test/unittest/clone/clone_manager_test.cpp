/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

#include "iremote_object.h"
#include "iremote_broker.h"
#include "message_parcel.h"

#include "auth_delegate.h"
#include "bootstrap.h"
#include "crypto/crypto_manager.h"
#include "device_manager_adapter.h"
#include "clone_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "message_parcel.h"
#include "serializable/serializable.h"
#include "utils/base64_utils.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using StoreMetaData = DistributedData::StoreMetaData;

namespace OHOS::Test {
const std::string SECRETKEY_BACKUP_PATH = "/data/service/el1/public/database/backup_test/";
const std::string SECRETKEY_BACKUP_FILE = SECRETKEY_BACKUP_PATH + "secret_key_backup.conf";
const std::string NORMAL_BACKUP_DATA =
    "{\"infos\":[{\"bundleName\":\"com.myapp.example\",\"dbName\":"
    "\"storeId\",\"instanceId\":0,\"storeType\":\"1\",\"user\":\"100\","
    "\"sKey\":\"9aJQwx3XD3EN7To2j/I9E9MCzn2+6f/bBqFjOPcY+1pRgx/"
    "XI6jXedyuzEEVdwrc\",\"time\":[50,180,137,103,0,0,0,0]},{"
    "\"bundleName\":\"sa1\",\"dbName\":\"storeId1\",\"instanceId\":\"0\","
    "\"dbType\":\"1\",\"user\":\"0\",\"sKey\":\"9aJQwx3XD3EN7To2j/"
    "I9E9MCzn2+6f/bBqFjOPcY+1pRgx/"
    "XI6jXedyuzEEVdwrc\",\"time\":[50,180,137,103,0,0,0,0]}]}";

class CloneManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void CloneManagerTest::SetUpTestCase(void)
{
    mode_t mode = S_IRWXU | S_IRWXG | S_IXOTH; // 0771
    mkdir(SECRETKEY_BACKUP_PATH.c_str(), mode);
    auto executors = std::make_shared<ExecutorPool>(12, 5);
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadDoubleSyncConfig();
}

void CloneManagerTest::TearDownTestCase(void)
{
    (void)remove(SECRETKEY_BACKUP_FILE.c_str());
    (void)rmdir(SECRETKEY_BACKUP_PATH.c_str());
}

void CloneManagerTest::SetUp(void)
{}

void CloneManagerTest::TearDown(void)
{}

static int32_t WriteContentToFile(const std::string &path, const std::string &content)
{
    std::fstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return -1;
    }
    file << content;
    file.close();
    return 0;
}

/**
 * @tc.name: OnRestore_ImportCloneKeyFailed
 * @tc.desc: Test OnRestore returns -1 when ImportCloneKey fails
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(CloneManagerTest, OnRestore_ImportCloneKeyFailed, TestSize.Level0)
{
    MessageParcel data;
    MessageParcel reply;

    // Prepare a valid backup file (content doesn't matter, as ImportCloneKey will fail first)
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, NORMAL_BACKUP_DATA), 0);
    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);

    // Prepare cloneInfoStr with invalid encryption_symkey (too short, so ImportCloneKey fails)
    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"1,2,3\",\"encryption_algname\":\"AES256\","
        "\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,106,25,231,15,15,""58,85,31,83,123,216,211,2,222,"
        "49,122,72,21,251,83,16\"}},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);

    // Should return -1 due to ImportCloneKey failure
    EXPECT_EQ(OnRestore(data, reply), -1);
    close(fd);
}

/**
 * @tc.name: ImportCloneKey_EmptyString
 * @tc.desc: Test ImportCloneKey with empty string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(CloneManagerTest, ImportCloneKey_EmptyString, TestSize.Level0)
{
    std::string emptyStr = "";
    EXPECT_FALSE(ImportCloneKey(emptyStr));
}

/**
 * @tc.name: isNumber_EmptyString
 * @tc.desc: Test IsNumber with empty string returns false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Claude
 */
HWTEST_F(CloneManagerTest, isNumber_EmptyString, TestSize.Level0)
{
    EXPECT_FALSE(IsNumber(""));
    EXPECT_TRUE(IsNumber("100"));
    EXPECT_TRUE(IsNumber("0"));
    EXPECT_TRUE(IsNumber("1234567890"));
    EXPECT_FALSE(IsNumber("abc"));
    EXPECT_FALSE(IsNumber("100abc"));
    EXPECT_FALSE(IsNumber("100.5"));
    EXPECT_FALSE(IsNumber("-100"));
    EXPECT_FALSE(IsNumber(" "));
}

/**
 * @tc.name: GetSecretKeyBackup_LoadMetaFail
 * @tc.desc: Test GetSecretKeyBackup when LoadMeta fails (continue branch)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Claude
 */
HWTEST_F(CloneManagerTest, GetSecretKeyBackup_LoadMetaFail, TestSize.Level0)
{
    std::vector<DistributedData::CloneBundleInfo> bundleInfos;
    DistributedData::CloneBundleInfo bundleInfo;
    bundleInfo.bundleName = "nonexistent.bundle";
    bundleInfo.accessTokenId = 100;
    bundleInfos.push_back(bundleInfo);

    std::vector<uint8_t> iv(32, 0);
    std::string userId = "100";
    std::string localDeviceId = "local_device_id";

    std::string result = GetSecretKeyBackup(bundleInfos, userId, iv, localDeviceId);
    // Should return serialized empty backup
    EXPECT_FALSE(result.empty());
}

/**
 * @tc.name: OnRestore_EmptyBackupData
 * @tc.desc: Test OnRestore when backupData.infos is empty (returns -1)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Claude
 */
HWTEST_F(CloneManagerTest, OnRestore_EmptyBackupData, TestSize.Level0)
{
    MessageParcel data;
    MessageParcel reply;

    // Create a backup file with empty infos
    std::string backupData = "{\"infos\":[]}";
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, backupData), 0);

    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68,188,239,20,170,227,190,37,142,218,"
        "158,177,32,5,160,13,114,186,141,59,91,44,200\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
        "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
        "16\"}},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);

    auto result = OnRestore(data, reply);
    EXPECT_EQ(result, -1);

    close(fd);
    ASSERT_EQ(remove(SECRETKEY_BACKUP_FILE.c_str()), 0);
}

/**
 * @tc.name: OnRestore_ParseFailure
 * @tc.desc: Test OnRestore when ParseSecretKeyFile fails
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Claude
 */
HWTEST_F(CloneManagerTest, OnRestore_ParseFailure, TestSize.Level0)
{
    MessageParcel data;
    MessageParcel reply;

    // Write invalid file descriptor
    data.WriteFileDescriptor(-1);
    data.WriteString("");

    auto result = OnRestore(data, reply);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: OnRestore_InvalidCloneInfo
 * @tc.desc: Test OnRestore when CloneInfo Unmarshal fails
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Claude
 */
HWTEST_F(CloneManagerTest, OnRestore_InvalidCloneInfo, TestSize.Level0)
{
    MessageParcel data;
    MessageParcel reply;

    // Create a valid backup file
    std::string backupData = "{\"infos\":[{\"bundleName\":\"test\",\"dbName\":\"test\","
        "\"user\":\"100\",\"sKey\":\"test\",\"instanceId\":0,\"storeType\":1}]}";
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, backupData), 0);

    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);

    // Write invalid clone info
    data.WriteString("InvalidJson");

    auto result = OnRestore(data, reply);
    EXPECT_EQ(result, -1);

    close(fd);
    ASSERT_EQ(remove(SECRETKEY_BACKUP_FILE.c_str()), 0);
}

/**
 * @tc.name: OnRestore_BackupItemInvalid
 * @tc.desc: Test OnRestore when BackupItem is invalid (continue loop)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Claude
 */
HWTEST_F(CloneManagerTest, OnRestore_BackupItemInvalid, TestSize.Level0)
{
    MessageParcel data;
    MessageParcel reply;

    // Create a backup file with invalid items
    std::string backupData = "{\"infos\":[{\"bundleName\":\"\",\"dbName\":\"\","
        "\"user\":\"\",\"sKey\":\"\",\"instanceId\":0,\"storeType\":1}]}";
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, backupData), 0);

    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68,188,239,20,170,227,190,37,142,218,"
        "158,177,32,5,160,13,114,186,141,59,91,44,200\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
        "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
        "16\"}},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);

    auto result = OnRestore(data, reply);
    // Should return 0 as invalid items are skipped
    EXPECT_EQ(result, 0);

    close(fd);
    ASSERT_EQ(remove(SECRETKEY_BACKUP_FILE.c_str()), 0);
}

/**
 * @tc.name: OnRestore_RestoreSecretKeyFailed
 * @tc.desc: Test OnRestore when RestoreSecretKey fails for some items (continue loop)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Claude
 */
HWTEST_F(CloneManagerTest, OnRestore_RestoreSecretKeyFailed, TestSize.Level0)
{
    MessageParcel data;
    MessageParcel reply;

    // Create a backup file with one valid and one invalid item
    std::string backupData = "{\"infos\":[""{\"bundleName\":\"test1\",\"dbName\":\"store1\","
        "\"user\":\"100\",\"sKey\":\"invalid\",\"instanceId\":0,\"storeType\":1},"
        "{\"bundleName\":\"test2\",\"dbName\":\"store2\","
        "\"user\":\"100\",\"sKey\":\"invalid\",\"instanceId\":0,\"storeType\":1}]}";
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, backupData), 0);

    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68,188,239,20,170,227,190,37,142,218,"
        "158,177,32,5,160,13,114,186,141,59,91,44,200\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
        "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
        "16\"}},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);

    auto result = OnRestore(data, reply);
    // Should return 0 as failed items are skipped
    EXPECT_EQ(result, 0);

    close(fd);
    ASSERT_EQ(remove(SECRETKEY_BACKUP_FILE.c_str()), 0);
}
} // namespace OHOS::Test