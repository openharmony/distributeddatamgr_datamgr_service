/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "preprocess_utils.h"
#include "gtest/gtest.h"
#include "text.h"
#include "unified_record.h"

namespace OHOS::UDMF {
using namespace testing::ext;
class UdmfPreProcessUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: RuntimeDataImputation001
* @tc.desc: Abnormal test of FillRuntimeInfo, option is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, RuntimeDataImputation001, TestSize.Level1)
{
    UnifiedData data;
    CustomOption option;
    PreProcessUtils preProcessUtils;
    DataLoadInfo dataLoadInfo;
    int32_t ret = preProcessUtils.FillRuntimeInfo(data, option);
    EXPECT_EQ(ret, E_ERROR);
}

/**
* @tc.name: DelayRuntimeDataImputation001
* @tc.desc: Abnormal test of DelayRuntimeDataImputation, option is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, DelayRuntimeDataImputation001, TestSize.Level1)
{
    UnifiedData data;
    CustomOption option;
    PreProcessUtils preProcessUtils;
    DataLoadInfo dataLoadInfo;
    int32_t ret = preProcessUtils.FillDelayRuntimeInfo(data, option, DataLoadInfo());
    EXPECT_EQ(ret, E_ERROR);
}

/**
* @tc.name: GetHapUidByToken001
* @tc.desc: Abnormal test of GetHapUidByToken, tokenId is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, GetHapUidByToken001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    int userId = 0;
    PreProcessUtils preProcessUtils;
    int32_t ret = preProcessUtils.GetHapUidByToken(tokenId, userId);
    EXPECT_EQ(ret, E_ERROR);
}

/**
* @tc.name: SetRemoteData001
* @tc.desc: Abnormal test of SetRemoteData, data is null
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, SetRemoteData001, TestSize.Level1)
{
    UnifiedData data;
    PreProcessUtils preProcessUtils;
    EXPECT_NO_FATAL_FAILURE(preProcessUtils.SetRemoteData(data));
}

/**
* @tc.name: SetRemoteData002
* @tc.desc: Normal test of SetRemoteData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, SetRemoteData002, TestSize.Level1)
{
    UnifiedData data;
    std::vector<std::shared_ptr<UnifiedRecord>> inputRecords;
    for (int32_t i = 0; i < 512; ++i) {
        inputRecords.emplace_back(std::make_shared<Text>());
    }
    data.SetRecords(inputRecords);
    data.runtime_ = std::make_shared<Runtime>();
    PreProcessUtils preProcessUtils;
    EXPECT_NO_FATAL_FAILURE(preProcessUtils.SetRemoteData(data));
}

/**
* @tc.name: GetDfsUrisFromLocal001
* @tc.desc: Abnormal test of GetDfsUrisFromLocal, uris is null
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, GetDfsUrisFromLocal001, TestSize.Level1)
{
    const std::vector<std::string> uris;
    int32_t userId = 0;
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    PreProcessUtils preProcessUtils;
    int32_t ret = preProcessUtils.GetDfsUrisFromLocal(uris, userId, dfsUris);
    EXPECT_EQ(ret, E_FS_ERROR);
}

/**
* @tc.name: CheckUriAuthorization001
* @tc.desc: Abnormal test of CheckUriAuthorization, uris is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, CheckUriAuthorization001, TestSize.Level1)
{
    const std::vector<std::string> uris = {"test"};
    uint32_t tokenId = 0;
    PreProcessUtils preProcessUtils;
    std::map<std::string, int32_t> permissionUris;
    bool ret = preProcessUtils.CheckUriAuthorization(uris, tokenId, permissionUris);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: ValidateUriScheme001
* @tc.desc: Abnormal test of ValidateUriScheme
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, ValidateUriScheme001, TestSize.Level1)
{
    std::string oriUri = "https://demo.com.html";
    Uri uri(oriUri);
    bool hasError = false;
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.ValidateUriScheme(uri, hasError);
    EXPECT_FALSE(ret);
    EXPECT_FALSE(hasError);
}

/**
* @tc.name: ValidateUriScheme002
* @tc.desc: Normal test of ValidateUriScheme
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, ValidateUriScheme002, TestSize.Level1)
{
    std::string oriUri = "file://ohos.test.demo1/data/storage/el2/base/haps/101.png";
    Uri uri(oriUri);
    bool hasError = false;
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.ValidateUriScheme(uri, hasError);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(hasError);
}

/**
* @tc.name: ValidateUriScheme003
* @tc.desc: Normal test of ValidateUriScheme
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, ValidateUriScheme003, TestSize.Level1)
{
    std::string oriUri = "file:///data/storage/el2/base/haps/101.png";
    Uri uri(oriUri);
    bool hasError = false;
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.ValidateUriScheme(uri, hasError);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(hasError);
}

/**
* @tc.name: ValidateUriScheme004
* @tc.desc: Normal test of ValidateUriScheme
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, ValidateUriScheme004, TestSize.Level1)
{
    std::string oriUri = "data/storage/el2/base/haps/101.png";
    Uri uri(oriUri);
    bool hasError = false;
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.ValidateUriScheme(uri, hasError);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(hasError);
}

/**
* @tc.name: GetInstIndex001
* @tc.desc: Normal test of GetInstIndex
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, GetInstIndex001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    int32_t instIndex = 0;
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.GetInstIndex(tokenId, instIndex);
    EXPECT_EQ(instIndex, 0);
    EXPECT_EQ(ret, true);
}

/**
* @tc.name: ProcessFileType001
* @tc.desc: Abnormal test of ProcessFileType, records is nullptr
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, ProcessFileType001, TestSize.Level1)
{
    std::shared_ptr<UnifiedRecord> record = std::make_shared<UnifiedRecord>();
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = "http://demo.com.html";
    obj->value_[FILE_TYPE] = "general.html";
    record->AddEntry("general.file-uri", obj);
    std::shared_ptr<Object> obj1 = std::make_shared<Object>();
    obj1->value_[UNIFORM_DATA_TYPE] = "general.content-form";
    obj1->value_["title"] = "test";
    record->AddEntry("general.content-form", obj1);

    std::shared_ptr<UnifiedRecord> record1 = std::make_shared<UnifiedRecord>();
    record1->AddEntry("general.file-uri", obj1);
    std::shared_ptr<UnifiedRecord> record2 = std::make_shared<UnifiedRecord>();
    record2->AddEntry("general.file-uri", "1111");
    std::shared_ptr<UnifiedRecord> record3 = std::make_shared<UnifiedRecord>();
    record3->AddEntry("general.file-uri", 1);
    std::vector<std::shared_ptr<UnifiedRecord>> records = { record, record1, record2, record3 };
    std::vector<std::string> uris;
    PreProcessUtils::ProcessFileType(records, [&uris](std::shared_ptr<Object> obj) {
        std::string oriUri;
        obj->GetValue(ORI_URI, oriUri);
        if (oriUri.empty()) {
            return false;
        }
        uris.push_back(oriUri);
        return true;
    });
    EXPECT_EQ(uris.size(), 1);
}

/**
* @tc.name: GetHtmlFileUris001
* @tc.desc: Abnormal test of GetHtmlFileUris, uris is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, GetHtmlFileUris001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    UnifiedData data;
    bool isLocal = false;
    std::map<std::string, int32_t> uris = { {"test", 0} };
    PreProcessUtils preProcessUtils;
    EXPECT_NO_FATAL_FAILURE(preProcessUtils.GetHtmlFileUris(tokenId, data, isLocal, uris));
}

/**
* @tc.name: SetRecordUid001
* @tc.desc: Abnormal test of SetRecordUid, record is nullptr
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, SetRecordUid001, TestSize.Level1)
{
    UnifiedData data;
    data.records_.emplace_back(nullptr);
    PreProcessUtils preProcessUtils;
    preProcessUtils.SetRecordUid(data);
    EXPECT_EQ(data.records_[0], nullptr);
}

/**
* @tc.name: FillUris001
* @tc.desc: Abnormal test of FillUris
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, FillUris001, TestSize.Level1)
{
    UnifiedData data;
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, int32_t> permissionUri;
    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    data.AddRecord(record);
    permissionUri.emplace("file://data/104.png", 1);
    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    std::shared_ptr<Object> obj1 = std::make_shared<Object>();
    obj1->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj1->value_[ORI_URI] = "file://data/103.png";
    auto record1 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj1);
    data.AddRecord(record1);
    std::shared_ptr<Object> obj2 = std::make_shared<Object>();
    obj2->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj2->value_[ORI_URI] = "file://data/104.png";
    auto record2 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj2);
    data.AddRecord(record2);
    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    int32_t permission;
    EXPECT_TRUE(obj2->GetValue(PERMISSION_POLICY, permission));
    EXPECT_EQ(permission, 1);
    AppFileService::ModuleRemoteFileShare::HmdfsUriInfo uriInfo = { "file://distributed/104.png", 1 };
    dfsUris.emplace("file://data/104.png", uriInfo);
    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    std::string dfsUri;
    EXPECT_TRUE(obj2->GetValue(REMOTE_URI, dfsUri));
    EXPECT_EQ(dfsUri, "file://distributed/104.png");
}

/**
* @tc.name: FillUris002
* @tc.desc: Abnormal test of FillUris
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, FillUris002, TestSize.Level1)
{
    UnifiedData data;
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, int32_t> permissionUri;
    permissionUri.emplace("file://authUri/104.png", 1);
    std::string html = "<img data-ohos='clipboard' src='file:///data/102.png'>"
                        "<img data-ohos='clipboard' src='file:///data/103.png'>";
    auto obj = std::make_shared<Object>();
    obj->value_["uniformDataType"] = "general.html";
    obj->value_["htmlContent"] = html;
    obj->value_["plainContent"] = "htmlPlainContent";
    auto record = std::make_shared<UnifiedRecord>(UDType::HTML, obj);
    data.AddRecord(record);
    std::map<std::string, int32_t> htmlUris;
    PreProcessUtils::GetHtmlFileUris(0, data, false, htmlUris);
    std::string key = "file:///data/102.png";
    record->ComputeUris([&key] (UriInfo &uriInfo) {
        if (uriInfo.oriUri == key) {
            uriInfo.authUri = "file://authUri/104.png";
        }
        return true;
    });
    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    int32_t permission;
    record->ComputeUris([&key, &permission] (UriInfo &uriInfo) {
        if (uriInfo.oriUri == key) {
            permission = uriInfo.permission;
        }
        return true;
    });
    EXPECT_EQ(permission, 1);
    AppFileService::ModuleRemoteFileShare::HmdfsUriInfo uriInfo = { "file://distributed/104.png", 0 };
    dfsUris.emplace("file://authUri/104.png", uriInfo);
    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    std::string dfsUri;
    record->ComputeUris([&key, &dfsUri] (UriInfo &uriInfo) {
        if (uriInfo.oriUri == key) {
            dfsUri = uriInfo.dfsUri;
        }
        return true;
    });
    EXPECT_EQ(dfsUri, "file://distributed/104.png");
}
}; // namespace UDMF