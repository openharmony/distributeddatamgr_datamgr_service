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

#include <dirent.h>
#include <thread>
#include "kvstore_account_observer.h"
#include "kvstore_data_service.h"
#include "refbase.h"
#include "types.h"
#include "bootstrap.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "common_event_manager.h"
#include "gtest/gtest.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using namespace OHOS;
using namespace OHOS::EventFwk;

static const int SYSTEM_USER_ID = 1000;

static const int WAIT_TIME_FOR_ACCOUNT_OPERATION = 2; // indicates the wait time in seconds

class DistributedDataAccountEventTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void ChangeUser(int uid);
    static void TearDownTestCase();
    static void HarmonyAccountLogin();
    static void HarmonyAccountLogout();
    static void HarmonyAccountDelete();
};

void DistributedDataAccountEventTest::SetUpTestCase()
{
    DistributedDataAccountEventTest::ChangeUser(SYSTEM_USER_ID);
}

void DistributedDataAccountEventTest::TearDownTestCase()
{
    DistributedDataAccountEventTest::HarmonyAccountDelete();
}

void DistributedDataAccountEventTest::HarmonyAccountLogin()
{
    Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_HWID_LOGIN);
    CommonEventData commonEventData(want);
    CommonEventData event(want);
    CommonEventPublishInfo publishInfo;
    auto err = CommonEventManager::PublishCommonEvent(event, publishInfo, nullptr);
    EXPECT_EQ(ERR_OK, err);
    sleep(WAIT_TIME_FOR_ACCOUNT_OPERATION);
}

void DistributedDataAccountEventTest::HarmonyAccountLogout()
{
    Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_HWID_LOGOUT);
    CommonEventData commonEventData(want);
    CommonEventData event(want);
    CommonEventPublishInfo publishInfo;
    auto err = CommonEventManager::PublishCommonEvent(event, publishInfo, nullptr);
    EXPECT_EQ(ERR_OK, err);
    sleep(WAIT_TIME_FOR_ACCOUNT_OPERATION);
}

void DistributedDataAccountEventTest::HarmonyAccountDelete()
{
    Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_HWID_TOKEN_INVALID);
    CommonEventData commonEventData(want);
    CommonEventData event(want);
    CommonEventPublishInfo publishInfo;
    auto err = CommonEventManager::PublishCommonEvent(event, publishInfo, nullptr);
    EXPECT_EQ(ERR_OK, err);
    sleep(WAIT_TIME_FOR_ACCOUNT_OPERATION);
}

void DistributedDataAccountEventTest::ChangeUser(int uid)
{
    if (setgid(uid)) {
        std::cout << "error to set gid " << uid << "errno is " << errno << std::endl;
    }

    if (setuid(uid)) {
        std::cout << "error to set uid " << uid << "errno is " << errno << std::endl;
    }
}

/**
 * @tc.name: Re-get exist SingleKvStore successfully verify when harmony account login/logout.
 * @tc.desc: Verify that after harmony account login/logout, re-operate exist SingleKvStore successfully.
 * @tc.type: FUNC
 * @tc.require: SR000DOH0F AR000DPSE7
 * @tc.author: FengLin
 */
HWTEST_F(DistributedDataAccountEventTest, GetKvStore_DefaultDeviceAccount_006, TestSize.Level3)
{
    Options options;
    options.createIfMissing = true;
    options.encrypt = false;
    options.autoSync = true;
    options.kvStoreType = KvStoreType::SINGLE_VERSION;

    AppId appId;
    appId.appId = "com.ohos.distributeddata.accountmsttest";
    StoreId storeId;
    storeId.storeId = "AccountEventStore006";
    std::string hashedStoreId;

    // Step1. Get KvStore with specified appId and storeId when harmony account logout.
    KvStoreDataService kvStoreDataService;
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
    sptr<ISingleKvStore> iSingleKvStorePtr;
    Status status = kvStoreDataService.GetSingleKvStore(options, appId, storeId,
                    [&](sptr<ISingleKvStore> singleKvStore) { iSingleKvStorePtr = std::move(singleKvStore); });
    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(iSingleKvStorePtr, nullptr) << "GetSingleKvStore executed failed";

    Key key = "Von";
    Value value = "Leon";
    status = iSingleKvStorePtr->Put(key, value);
    EXPECT_EQ(status, Status::SUCCESS) << "PUT string to SingleKvStore return wrong status";
    Value getValue = "";
    iSingleKvStorePtr->Get(key, getValue);
    EXPECT_EQ(getValue, value) << "Get string from SingleKvStore not equal to PUT";

    // Step2. Get ResultSet and verify get entry from SingleKvStore correctly.
    sptr<IKvStoreResultSet> iKvStoreResultSet;
    auto resultSetCallbackFunction = [&](Status statusTmp, sptr<IKvStoreResultSet> iKvStoreResultSetTmp) {
        status = statusTmp;
        iKvStoreResultSet = iKvStoreResultSetTmp;
    };
    Key prefixKey = "Von";
    Entry entry;
    iSingleKvStorePtr->GetResultSet(prefixKey, resultSetCallbackFunction);
    EXPECT_EQ(status, Status::SUCCESS) << "Get resultset from SingleKvStore return wrong status";
    iKvStoreResultSet->MoveToNext();
    status = iKvStoreResultSet->GetEntry(entry);
    EXPECT_EQ(status, Status::SUCCESS) << "Get entry from ResultSet return wrong status";
    EXPECT_EQ(entry.key, key) << "Get entry key from SingleKvStore not equal to PUT";
    EXPECT_EQ(entry.value, value) << "Get entry value from SingleKvStore not equal to PUT";

    sleep(WAIT_TIME_FOR_ACCOUNT_OPERATION);
    // Step3. Harmony account login
    DistributedDataAccountEventTest::HarmonyAccountLogin();

    // Step4. Verify get value from SingleKvStore and put to SingleKvStore with last resultset correctly after LOGIN.
    getValue.Clear();
    iSingleKvStorePtr->Get(key, getValue);
    EXPECT_EQ(getValue, value) << "Get string from SingleKvStore not equal to PUT";

    Entry entry2;
    iKvStoreResultSet->GetEntry(entry2);
    EXPECT_EQ(entry2.key, key) << "Get entry key from SingleKvStore not equal to PUT";
    EXPECT_EQ(entry2.value, value) << "Get entry value from SingleKvStore not equal to PUT";

    Key key2 = "Von2";
    Value value2 = "Leon2";
    status = iSingleKvStorePtr->Put(key2, value2);
    EXPECT_EQ(status, Status::SUCCESS) << "PUT string to SingleKvStore return wrong status";
    getValue.Clear();
    iSingleKvStorePtr->Get(key2, getValue);
    EXPECT_EQ(getValue, value2) << "Get string from SingleKvStore not equal to PUT after LOGIN";

    Entry entry3;
    iSingleKvStorePtr->CloseResultSet(iKvStoreResultSet);
    iSingleKvStorePtr->GetResultSet(prefixKey, resultSetCallbackFunction);
    EXPECT_EQ(status, Status::SUCCESS) << "Get resultset from SingleKvStore return wrong status";
    iKvStoreResultSet->MoveToNext();
    status = iKvStoreResultSet->GetEntry(entry3);
    EXPECT_EQ(status, Status::SUCCESS) << "Get entry from SingleKvStore return wrong status";
    EXPECT_EQ(entry3.key, key) << "Get entry key from SingleKvStore not equal to PUT";
    EXPECT_EQ(entry3.value, value) << "Get entry value from SingleKvStore not equal to PUT";

    // Step6. Harmony account logout
    DistributedDataAccountEventTest::HarmonyAccountLogout();

    // Step7. Verify get value from SingleKvStore and put to SingleKvStore with last resultset correctly after LOGOUT.
    getValue.Clear();
    iSingleKvStorePtr->Get(key, getValue);
    EXPECT_EQ(getValue, value) << "Get string from SingleKvStore not equal to PUT";

    Entry entry4;
    iKvStoreResultSet->GetEntry(entry4);
    EXPECT_EQ(entry4.key, key) << "Get entry key from SingleKvStore not equal to PUT";
    EXPECT_EQ(entry4.value, value) << "Get entry value from SingleKvStore not equal to PUT";

    Key key3 = "Von3";
    Value value3 = "Leon3";
    status = iSingleKvStorePtr->Put(key3, value3);
    EXPECT_EQ(status, Status::SUCCESS) << "PUT string to SingleKvStore return wrong status";
    getValue.Clear();
    iSingleKvStorePtr->Get(key3, getValue);
    EXPECT_EQ(getValue, value3) << "Get string from SingleKvStore not equal to PUT after LOGIN";

    Entry entry5;
    iSingleKvStorePtr->GetResultSet(prefixKey, resultSetCallbackFunction);
    iKvStoreResultSet->MoveToNext();
    iKvStoreResultSet->GetEntry(entry5);
    EXPECT_EQ(entry5.key, key) << "Get entry key from SingleKvStore not equal to PUT";
    EXPECT_EQ(entry5.value, value) << "Get entry value from SingleKvStore not equal to PUT";

    iSingleKvStorePtr->CloseResultSet(iKvStoreResultSet);
    status = kvStoreDataService.CloseKvStore(appId, storeId);
    EXPECT_EQ(status, Status::SUCCESS) << "CloseKvStore return wrong status after harmony account logout";

    // Step8. Verify that when harmony account logout and in the situation that the exist KvStore has been closed,
    // re-get exist KvStore successfully and the iSingleKvStorePtr not equal with the one before harmony account logout.
    sptr<ISingleKvStore> iSingleKvStoreLogoutPtr;
    status = kvStoreDataService.GetSingleKvStore(options, appId, storeId,
             [&](sptr<ISingleKvStore> singleKvStore) { iSingleKvStoreLogoutPtr = std::move(singleKvStore); });
    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status after harmony account logout";
    EXPECT_NE(iSingleKvStoreLogoutPtr, nullptr) << "GetSingleKvStore executed failed after harmony account logout";
    EXPECT_NE(iSingleKvStorePtr, iSingleKvStoreLogoutPtr) << "iSingleKvStorePtr NE fail after harmony account logout";
    status = kvStoreDataService.CloseKvStore(appId, storeId);
    EXPECT_EQ(status, Status::SUCCESS) << "CloseKvStore return wrong status after harmony account logout";
}