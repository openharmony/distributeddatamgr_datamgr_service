/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "UdmfServiceImplTest"
#include "udmf_service_impl.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "kvstore_meta_manager.h"
#include "metadata/meta_data_manager.h"
#include "nativetoken_kit.h"
#include "preprocess_utils.h"
#include "runtime_store.h"
#include "text.h"
#include "plain_text.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Entry = DistributedDB::Entry;
using Key = DistributedDB::Key;
using Value = DistributedDB::Value;
using UnifiedData = OHOS::UDMF::UnifiedData;
using Summary =  OHOS::UDMF::Summary;
namespace OHOS::Test {
namespace DistributedDataTest {

static void GrantPermissionNative()
{
    const char **perms = new const char *[3];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    perms[2] = "ohos.permission.MONITOR_DEVICE_NETWORK_STATE"; // perms[2] is a permission parameter
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 3,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_data_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

class UdmfServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GrantPermissionNative();
        DistributedData::Bootstrap::GetInstance().LoadComponents();
        DistributedData::Bootstrap::GetInstance().LoadDirectory();
        DistributedData::Bootstrap::GetInstance().LoadCheckers();
        size_t max = 2;
        size_t min = 1;
        auto executors = std::make_shared<OHOS::ExecutorPool>(max, min);
        DmAdapter::GetInstance().Init(executors);
        DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
        DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
        DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    }
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};

    const std::string STORE_ID = "drag";
};

/**
* @tc.name: SaveData001
* @tc.desc: Abnormal test of SaveData, unifiedData is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SaveData001, TestSize.Level1)
{
    CustomOption option;
    UnifiedData data;
    std::string key = "";
    option.intention = UD_INTENTION_BASE;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SaveData(option, data, key);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: RetrieveData001
* @tc.desc: Abnormal test of RetrieveData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, RetrieveData001, TestSize.Level1)
{
    QueryOption query;
    UnifiedData data;
    query.intention = UD_INTENTION_BASE;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.RetrieveData(query, data);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: IsPermissionInCache002
* @tc.desc: Abnormal test of IsPermissionInCache, privilegeCache_ has no query.key
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, IsPermissionInCache002, TestSize.Level1)
{
    QueryOption query;
    UdmfServiceImpl udmfServiceImpl;
    bool ret = udmfServiceImpl.IsPermissionInCache(query);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: UpdateData001
* @tc.desc: Abnormal test of UpdateData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, UpdateData001, TestSize.Level1)
{
    QueryOption query;
    UnifiedData data;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.UpdateData(query, data);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: GetSummary001
* @tc.desc: Abnormal test of UpdateData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, GetSummary001, TestSize.Level1)
{
    QueryOption query;
    Summary summary;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.GetSummary(query, summary);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: Sync001
* @tc.desc: Abnormal test of Sync, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, Sync001, TestSize.Level1)
{
    QueryOption query;
    std::vector<std::string> devices = {"device1"};
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.Sync(query, devices);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: IsRemoteData001
* @tc.desc: Abnormal test of IsRemoteData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, IsRemoteData001, TestSize.Level1)
{
    QueryOption query;
    bool result = false;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.IsRemoteData(query, result);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption001
* @tc.desc: Abnormal test of SetAppShareOption, intention is empty
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption001, TestSize.Level1)
{
    std::string intention = "";
    int32_t shareOption = 1;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}


/**
* @tc.name: SetAppShareOption002
* @tc.desc: Abnormal test of SetAppShareOption, shareOption > SHARE_OPTIONS_BUTT
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption002, TestSize.Level1)
{
    std::string intention = "intention";
    int32_t shareOption = 4;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption003
* @tc.desc: Abnormal test of SetAppShareOption, shareOption = SHARE_OPTIONS_BUTT
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption003, TestSize.Level1)
{
    std::string intention = "intention";
    int32_t shareOption = 3;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption004
* @tc.desc: Abnormal test of SetAppShareOption, shareOption < IN_APP
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption004, TestSize.Level1)
{
    std::string intention = "intention";
    int32_t shareOption = -1;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: OnUserChangeTest001
* @tc.desc: OnUserChange test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, OnUserChangeTest001, TestSize.Level1)
{
    uint32_t code = 4;
    std::string user = "OH_USER_test";
    std::string account = "OH_ACCOUNT_test";
    UdmfServiceImpl udmfServiceImpl;
    auto status = udmfServiceImpl.OnUserChange(code, user, account);
    ASSERT_EQ(status, UDMF::E_OK);
    auto sizeAfter = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(sizeAfter, 0);
}

/**
* @tc.name: SaveMetaData001
* @tc.desc: Abnormal testcase of GetRuntime
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SaveMetaData001, TestSize.Level0)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    
    result = store->Init();
    EXPECT_TRUE(result);
}

/**
* @tc.name: SaveMetaData001
* @tc.desc: Abnormal testcase of GetRuntime
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SyncTest001, TestSize.Level0)
{
    QueryOption query;
    query.key = "udmf://drag/ohos.test.demo1/_aS6adWi7<Dehfffffffffffffffff";
    query.tokenId = 1;
    query.intention  = UD_INTENTION_DRAG;
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    auto ret = udmfServiceImpl.Sync(query, devices);
    EXPECT_EQ(ret, UDMF::E_DB_ERROR);
}

/**
* @tc.name: ResolveAutoLaunch001
* @tc.desc: test ResolveAutoLaunch
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, ResolveAutoLaunch001, TestSize.Level0)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    
    DistributedDB::AutoLaunchParam param {
        .userId = "100",
        .appId = "distributeddata",
        .storeId = "drag",
    };
    std::string identifier = "identifier";
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    auto ret = udmfServiceImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(ret, UDMF::E_OK);
}

/**
* @tc.name: TransferToEntriesIfNeedTest001
* @tc.desc: TransferToEntriesIfNeed test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, TransferToEntriesIfNeedTest001, TestSize.Level1)
{
    UnifiedData data;
    QueryOption query;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    data.AddRecord(record1);
    data.AddRecord(record2);
    auto properties = std::make_shared<UnifiedDataProperties>();
    properties->tag = "records_to_entries_data_format";
    data.SetProperties(properties);
    query.tokenId = 1;
    UdmfServiceImpl udmfServiceImpl;
    udmfServiceImpl.TransferToEntriesIfNeed(query, data);
    EXPECT_TRUE(data.IsNeedTransferToEntries());
    int recordSize = 2;
    EXPECT_EQ(data.GetRecords().size(), recordSize);
}
/**
 * @tc.name: IsValidInput001
 * @tc.desc: invalid unifiedData
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput001, TestSize.Level1)
{
    QueryOption query;
    UnifiedData unifiedData;
    UnifiedKey key;

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidInput002
 * @tc.desc: invalid key
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput002, TestSize.Level1)
{
    QueryOption query;
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    UnifiedKey key;

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidInput003
 * @tc.desc: valid input
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput003, TestSize.Level1)
{
    QueryOption query;
    query.intention = Intention::UD_INTENTION_DATA_HUB;
    query.key = "udmf://DataHub/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7";
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    UnifiedKey key("udmf://DataHub/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7");
    EXPECT_TRUE(key.IsValid());

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsValidInput004
 * @tc.desc: invalid intention
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput004, TestSize.Level1)
{
    QueryOption query;
    query.intention = Intention::UD_INTENTION_DRAG;
    UnifiedData unifiedData;
    UnifiedKey key("udmf://drag/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7");

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAppId001
 * @tc.desc: invalid bundleName
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, CheckAppId001, TestSize.Level1)
{
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->appId = "appId";
    std::string bundleName = "ohos.test.demo1";

    UdmfServiceImpl impl;
    int32_t result = impl.CheckAppId(runtime, bundleName);
    EXPECT_EQ(result, E_INVALID_PARAMETERS);
}

/**
 * @tc.name: CheckAppId002
 * @tc.desc: invalid runtime
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, CheckAppId002, TestSize.Level1)
{
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    std::string bundleName = "ohos.test.demo1";

    UdmfServiceImpl impl;
    int32_t result = impl.CheckAppId(runtime, bundleName);
    EXPECT_EQ(result, E_INVALID_PARAMETERS);
}

/**
 * @tc.name: ValidateAndProcessRuntimeData001
 * @tc.desc: invalid appId
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ValidateAndProcessRuntimeData001, TestSize.Level1)
{
    UnifiedData data;
    std::shared_ptr<UnifiedRecord> record = std::make_shared<PlainText>(UDType::PLAIN_TEXT, "plainTextContent");
    data.AddRecord(record);
    std::shared_ptr<UnifiedDataProperties> properties = std::make_shared<UnifiedDataProperties>();
    std::string tag = "this is a tag of test ValidateAndProcessRuntimeData001";
    properties->tag = tag;
    data.SetProperties(std::move(properties));

    std::vector<UnifiedData> dataSet = {data};
    std::shared_ptr<Runtime> runtime;
    std::vector<UnifiedData> unifiedDataSet;
    std::vector<std::string> deleteKeys;
    QueryOption query;

    UdmfServiceImpl impl;
    int32_t result = impl.ValidateAndProcessRuntimeData(dataSet, runtime, unifiedDataSet, query, deleteKeys);
    EXPECT_EQ(result, UDMF::E_DB_ERROR);
}

/**
 * @tc.name: ValidateAndProcessRuntimeData002
 * @tc.desc: Normal test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ValidateAndProcessRuntimeData002, TestSize.Level1)
{
    UnifiedData data;
    data.runtime_ = std::make_shared<Runtime>();
    data.runtime_->tokenId = 1;
    std::shared_ptr<UnifiedRecord> record = std::make_shared<PlainText>(UDType::PLAIN_TEXT, "plainTextContent");
    data.AddRecord(record);
    std::shared_ptr<UnifiedDataProperties> properties = std::make_shared<UnifiedDataProperties>();
    std::string tag = "this is a tag of test ValidateAndProcessRuntimeData002";
    properties->tag = tag;
    data.SetProperties(std::move(properties));

    std::vector<UnifiedData> dataSet = {data};
    std::shared_ptr<Runtime> runtime;
    std::vector<UnifiedData> unifiedDataSet;
    std::vector<std::string> deleteKeys;
    QueryOption query;
    query.tokenId = 1;

    UdmfServiceImpl impl;
    int32_t result = impl.ValidateAndProcessRuntimeData(dataSet, runtime, unifiedDataSet, query, deleteKeys);
    EXPECT_EQ(result, UDMF::E_OK);
}

/**
 * @tc.name: ValidateAndProcessRuntimeData003
 * @tc.desc: invalid appId
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ValidateAndProcessRuntimeData003, TestSize.Level1)
{
    UnifiedData data;
    data.runtime_ = std::make_shared<Runtime>();
    data.runtime_->tokenId = 1;
    data.runtime_ = std::make_shared<Runtime>();
    data.runtime_->appId = "appId";
    std::shared_ptr<UnifiedRecord> record = std::make_shared<PlainText>(UDType::PLAIN_TEXT, "plainTextContent");
    data.AddRecord(record);
    std::shared_ptr<UnifiedDataProperties> properties = std::make_shared<UnifiedDataProperties>();
    std::string tag = "this is a tag of test ValidateAndProcessRuntimeData003";
    properties->tag = tag;
    data.SetProperties(std::move(properties));

    std::vector<UnifiedData> dataSet = {data};
    std::shared_ptr<Runtime> runtime;
    std::vector<UnifiedData> unifiedDataSet;
    std::vector<std::string> deleteKeys;
    QueryOption query;
    query.tokenId = 2;

    UdmfServiceImpl impl;
    int32_t result = impl.ValidateAndProcessRuntimeData(dataSet, runtime, unifiedDataSet, query, deleteKeys);
    EXPECT_EQ(result, UDMF::E_OK);
}

/**
 * @tc.name: ValidateAndProcessRuntimeData004
 * @tc.desc: invalid appId
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ValidateAndProcessRuntimeData004, TestSize.Level1)
{
    UnifiedData data1;
    data1.runtime_ = std::make_shared<Runtime>();
    data1.runtime_->tokenId = 1;
    data1.runtime_ = std::make_shared<Runtime>();
    data1.runtime_->appId = "appId";
    std::shared_ptr<UnifiedRecord> record = std::make_shared<PlainText>(UDType::PLAIN_TEXT, "plainTextContent");
    data1.AddRecord(record);
    std::shared_ptr<UnifiedDataProperties> properties = std::make_shared<UnifiedDataProperties>();
    std::string tag = "this is a tag of test ValidateAndProcessRuntimeData004";
    properties->tag = tag;
    data1.SetProperties(std::move(properties));

    UnifiedData data2;
    data2.runtime_ = std::make_shared<Runtime>();
    data2.runtime_->tokenId = 1;
    data2.runtime_ = std::make_shared<Runtime>();
    data2.runtime_->appId = "appId";
    data2.AddRecord(record);
    data2.SetProperties(std::move(properties));

    std::vector<UnifiedData> dataSet = { data1, data2 };
    std::shared_ptr<Runtime> runtime;
    std::vector<UnifiedData> unifiedDataSet;
    std::vector<std::string> deleteKeys;
    QueryOption query;
    query.tokenId = 2;

    UdmfServiceImpl impl;
    int32_t result = impl.ValidateAndProcessRuntimeData(dataSet, runtime, unifiedDataSet, query, deleteKeys);
    EXPECT_EQ(result, UDMF::E_OK);
}

/**
 * @tc.name: CloseStoreWhenCorrupted001
 * @tc.desc: Normal test of CloseStoreWhenCorrupted
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, CloseStoreWhenCorrupted001, TestSize.Level1)
{
    std::string intention = "drag";
    StoreCache::GetInstance().CloseStores();
    auto store = StoreCache::GetInstance().GetStore(intention);
    EXPECT_EQ(StoreCache::GetInstance().stores_.Size(), 1);
    int32_t status = UDMF::E_OK;
    UdmfServiceImpl impl;
    impl.HandleDbError(intention, status);
    EXPECT_EQ(StoreCache::GetInstance().stores_.Size(), 1);
}

/**
 * @tc.name: CloseStoreWhenCorrupted002
 * @tc.desc: Abnormal test of CloseStoreWhenCorrupted
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, CloseStoreWhenCorrupted002, TestSize.Level1)
{
    std::string intention = "drag";
    StoreCache::GetInstance().CloseStores();
    auto store = StoreCache::GetInstance().GetStore(intention);
    EXPECT_EQ(StoreCache::GetInstance().stores_.Size(), 1);
    int32_t status = UDMF::E_DB_CORRUPTED;
    UdmfServiceImpl impl;
    impl.HandleDbError(intention, status);
    EXPECT_EQ(StoreCache::GetInstance().stores_.Size(), 0);
    EXPECT_EQ(status, UDMF::E_DB_ERROR);
}
}; // namespace DistributedDataTest
}; // namespace OHOS::Test
