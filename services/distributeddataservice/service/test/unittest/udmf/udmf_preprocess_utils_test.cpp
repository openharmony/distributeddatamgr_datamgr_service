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
#include "unified_html_record_process.h"
#include "unified_record.h"
#include "uri_permission_util.h"

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
* @tc.name: SetRemoteData003
* @tc.desc: Normal test of SetRemoteData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, SetRemoteData003, TestSize.Level1)
{
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = "file://demo.com/a.png";
    obj->value_[FILE_TYPE] = "abcdefg";
    obj->value_[REMOTE_URI] = "abcdefg";
    std::shared_ptr<Object> detailObj = std::make_shared<Object>();
    detailObj->value_[TITLE] = "title";
    obj->value_[DETAILS] = detailObj;
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data.AddRecord(record);
    Runtime runtime;
    runtime.deviceId = "remote";
    data.SetRuntime(runtime);
    PreProcessUtils::SetRemoteData(data);
    auto remoteUriGet = std::get<std::string>(obj->value_[REMOTE_URI]);
    EXPECT_EQ(remoteUriGet, "abcdefg");
    auto detailGet = std::get<std::shared_ptr<Object>>(obj->value_[DETAILS]);
    auto isRemote = std::get<std::string>(detailGet->value_["isRemote"]);
    EXPECT_EQ(isRemote, "true");
}

/**
* @tc.name: SetRemoteData004
* @tc.desc: Normal test of SetRemoteData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, SetRemoteData004, TestSize.Level1)
{
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = "file://demo.com/a.png";
    obj->value_[FILE_TYPE] = "abcdefg";
    obj->value_[REMOTE_URI] = "abcdefg";
    std::shared_ptr<Object> detailObj = std::make_shared<Object>();
    detailObj->value_[TITLE] = "title";
    obj->value_[DETAILS] = detailObj;
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data.AddRecord(record);
    Runtime runtime;
    runtime.deviceId = PreProcessUtils::GetLocalDeviceId();
    data.SetRuntime(runtime);
    PreProcessUtils::SetRemoteData(data);
    auto remoteUriGet = std::get<std::string>(obj->value_[REMOTE_URI]);
    EXPECT_TRUE(remoteUriGet.empty());
    auto detailGet = std::get<std::shared_ptr<Object>>(obj->value_[DETAILS]);
    EXPECT_TRUE(detailGet->value_.find("isRemote") == detailGet->value_.end());
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
    std::map<std::string, uint32_t> permissionUris;
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
    std::shared_ptr<UnifiedRecord> record4 = nullptr;
    std::shared_ptr<Object> obj2 = std::make_shared<Object>();
    std::shared_ptr<UnifiedRecord> record5 = std::make_shared<UnifiedRecord>();
    record3->AddEntry("general.file-uri", obj2);
    std::vector<std::shared_ptr<UnifiedRecord>> records = { record, record1, record2, record3, record4, record5 };
    std::vector<std::string> uris;
    for (const auto &item : records) {
        PreProcessUtils::ProcessFileType(item, [&uris](std::shared_ptr<Object> obj) {
            std::string oriUri;
            obj->GetValue(ORI_URI, oriUri);
            if (oriUri.empty()) {
                return false;
            }
            uris.push_back(oriUri);
            return true;
        });
    }
    EXPECT_EQ(uris.size(), 1);
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
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, std::make_shared<Object>());
    (*record->GetEntries())[ORI_URI] = "file://test.com/data.txt";
    data.AddRecord(record);
    
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, uint32_t> permissionUris;
    
    PreProcessUtils preProcessUtils;
    preProcessUtils.FillUris(data, dfsUris, permissionUris);
    EXPECT_TRUE(dfsUris.empty());
}

/**
 * @tc.name: CheckUriAuthorization002
 * @tc.desc: Test CheckUriAuthorization with valid URIs
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, CheckUriAuthorization002, TestSize.Level1)
{
    const std::vector<std::string> uris = {"file://test.com/data.txt"};
    uint32_t tokenId = 100;
    std::map<std::string, uint32_t> permissionUris;
    
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.CheckUriAuthorization(uris, tokenId, permissionUris, false);
    EXPECT_FALSE(ret);
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
    std::map<std::string, uint32_t> permissionUri;
    permissionUri.emplace("file://authUri/104.png", UriPermissionUtil::READ_FLAG);
    std::string html = "<img data-ohos='clipboard' src='file:///data/102.png'>"
                        "<img data-ohos='clipboard' src='file:///data/103.png'>";
    auto obj = std::make_shared<Object>();
    obj->value_["uniformDataType"] = "general.html";
    obj->value_["htmlContent"] = html;
    obj->value_["plainContent"] = "htmlPlainContent";
    auto record = std::make_shared<UnifiedRecord>(UDType::HTML, obj);
    data.AddRecord(record);
    UnifiedHtmlRecordProcess::GetUriFromHtmlRecord(*record);
    std::string key = "file:///data/102.png";
    record->ComputeUris([&key] (UriInfo &uriInfo) {
        if (uriInfo.oriUri == key) {
            uriInfo.authUri = "file://authUri/104.png";
        }
        return true;
    });
    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    int32_t permission;
    uint32_t permissionMask = 0;
    record->ComputeUris([&key, &permission] (UriInfo &uriInfo) {
        if (uriInfo.oriUri == key) {
            permission = uriInfo.permission;
        }
        return true;
    });
    EXPECT_EQ(permission, static_cast<int32_t>(PermissionPolicy::ONLY_READ));
    record->ComputeUris([&key, &permissionMask] (UriInfo &uriInfo) {
        if (uriInfo.oriUri == key) {
            permissionMask = uriInfo.permissionMask;
        }
        return true;
    });
    EXPECT_EQ(permissionMask, UriPermissionUtil::READ_FLAG);
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

/**
* @tc.name: FillUris003
* @tc.desc: New permission version should keep exact permission policy
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, FillUris003, TestSize.Level1)
{
    UnifiedData data;
    Runtime runtime;
    runtime.permissionPolicyMode = PERMISSION_POLICY_MODE_MASK;
    data.SetRuntime(runtime);
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, uint32_t> permissionUri;
    permissionUri.emplace("file://data/105.png", UriPermissionUtil::WRITE_FLAG | UriPermissionUtil::PERSIST_FLAG);

    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[ORI_URI] = "file://data/105.png";
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    data.AddRecord(record);

    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    int32_t permission = static_cast<int32_t>(PermissionPolicy::NO_PERMISSION);
    EXPECT_TRUE(obj->GetValue(PERMISSION_POLICY, permission));
    EXPECT_EQ(permission, static_cast<int32_t>(PermissionPolicy::READ_WRITE));
    int32_t permissionExt = 0;
    EXPECT_TRUE(obj->GetValue(URI_PERMISSION_MASK, permissionExt));
    EXPECT_EQ(permissionExt, static_cast<int32_t>(UriPermissionUtil::WRITE_FLAG | UriPermissionUtil::PERSIST_FLAG));
}

/**
* @tc.name: FillUris005
* @tc.desc: FillUris should write final authorized permission range
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, FillUris005, TestSize.Level1)
{
    UnifiedData data;
    Runtime runtime;
    runtime.permissionPolicyMode = PERMISSION_POLICY_MODE_MASK;
    data.SetRuntime(runtime);
    auto properties = data.GetProperties();
    properties->uriAuthorizationPolicies = { UriPermission::READ };
    data.SetProperties(std::move(properties));
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, uint32_t> permissionUri;
    permissionUri.emplace("file://data/107.png",
        UriPermissionUtil::READ_FLAG | UriPermissionUtil::WRITE_FLAG | UriPermissionUtil::PERSIST_FLAG);

    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[ORI_URI] = "file://data/107.png";
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    data.AddRecord(record);

    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    int32_t permission = static_cast<int32_t>(PermissionPolicy::NO_PERMISSION);
    EXPECT_TRUE(obj->GetValue(PERMISSION_POLICY, permission));
    EXPECT_EQ(permission, static_cast<int32_t>(PermissionPolicy::ONLY_READ));
    int32_t permissionExt = 0;
    EXPECT_TRUE(obj->GetValue(URI_PERMISSION_MASK, permissionExt));
    EXPECT_EQ(permissionExt, static_cast<int32_t>(UriPermissionUtil::READ_FLAG));
}

/**
* @tc.name: FillUris004
* @tc.desc: Legacy permission version should keep old authorization compatibility
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, FillUris004, TestSize.Level1)
{
    UnifiedData data;
    Runtime runtime;
    runtime.permissionPolicyMode = PERMISSION_POLICY_MODE_LEGACY;
    data.SetRuntime(runtime);
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, uint32_t> permissionUri;
    permissionUri.emplace("file://data/106.png", UriPermissionUtil::WRITE_FLAG | UriPermissionUtil::PERSIST_FLAG);

    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[ORI_URI] = "file://data/106.png";
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    data.AddRecord(record);

    PreProcessUtils::FillUris(data, dfsUris, permissionUri);
    int32_t permission = static_cast<int32_t>(PermissionPolicy::NO_PERMISSION);
    EXPECT_TRUE(obj->GetValue(PERMISSION_POLICY, permission));
    EXPECT_EQ(permission, static_cast<int32_t>(PermissionPolicy::READ_WRITE));
    int32_t permissionExt = 0;
    EXPECT_TRUE(obj->GetValue(URI_PERMISSION_MASK, permissionExt));
    EXPECT_EQ(permissionExt, static_cast<int32_t>(UriPermissionUtil::WRITE_FLAG | UriPermissionUtil::PERSIST_FLAG));
}

/**
* @tc.name: MatchImgExtension001
* @tc.desc: Normal testcase of MatchImgExtension
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsTest, MatchImgExtension001, TestSize.Level1)
{
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///example.com/img1.png"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///example.com/img1.jpg"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///example.com/img1.jpeg"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///example.com/img1.PNG"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///example.com/img1.JPG"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///example.com/img1.JPEG"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///1.png"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///1.jpg"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///aaa/bbb/ccc.jpeg"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///aaa/bbb/ccc.gif"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///aaa/bbb/ccc.gif?query=aaa"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///aaa/bbb/ccc.gif;version=1"));
    EXPECT_TRUE(PreProcessUtils::MatchImgExtension("file:///aaa/bbb/ccc.gif;version=1?query=aaa"));
}

/**
* @tc.name: MatchImgExtension002
* @tc.desc: Abnormal testcase of MatchImgExtension
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsTest, MatchImgExtension002, TestSize.Level1)
{
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension(""));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:////"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///./"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:////."));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///png"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///png."));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///png/"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///png/bbb"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///aaa/bbb/.png/ccc"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///aaa/bbb/1.png/ccc"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///example.com/img1.invalidext"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///test.com/img2jpg"));
    EXPECT_FALSE(PreProcessUtils::MatchImgExtension("file:///test.com/"));
}

/**
 * @tc.name: HandleFileUris001
 * @tc.desc: Test HandleFileUris with valid file URIs
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, HandleFileUris001, TestSize.Level1)
{
    UnifiedData data;
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, std::make_shared<Object>());
    Runtime runtime;
    runtime.permissionPolicyMode = PERMISSION_POLICY_MODE_LEGACY;
    data.SetRuntime(runtime);
    (*record->GetEntries())[ORI_URI] = "file://test.com/data.txt";
    data.AddRecord(record);
    
    PreProcessUtils preProcessUtils;
    int32_t ret = preProcessUtils.HandleFileUris(100, data);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: ReadCheckUri001
 * @tc.desc: Test ReadCheckUri with valid URIs
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, ReadCheckUri001, TestSize.Level1)
{
    UnifiedData data;
    Runtime runtime;
    runtime.permissionPolicyMode = PERMISSION_POLICY_MODE_LEGACY;
    data.SetRuntime(runtime);
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, std::make_shared<Object>());
    (*record->GetEntries())[ORI_URI] = "file://test.com/data.txt";
    data.AddRecord(record);

    std::vector<std::string> uris = {"file://test.com/data.txt"};
    PreProcessUtils preProcessUtils;
    int32_t ret = preProcessUtils.ReadCheckUri(100, data, uris, false);
    EXPECT_EQ(ret, E_NO_PERMISSION);
}

/**
 * @tc.name: FillHtmlEntry001
 * @tc.desc: Test FillHtmlEntry with HTML record
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, FillHtmlEntry001, TestSize.Level1)
{
    auto record = std::make_shared<UnifiedRecord>(UDType::HTML, std::make_shared<Object>());
    auto properties = std::make_shared<UnifiedDataProperties>();
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, uint32_t> permissionUris;
    
    PreProcessUtils preProcessUtils;
    preProcessUtils.FillHtmlEntry(record, properties, dfsUris, permissionUris);
    EXPECT_TRUE(dfsUris.empty());
}

/**
 * @tc.name: FillUris006
 * @tc.desc: Test FillUris with valid data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, FillUris006, TestSize.Level1)
{
    UnifiedData data;
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, std::make_shared<Object>());
    (*record->GetEntries())[ORI_URI] = "file://test.com/data.txt";
    data.AddRecord(record);
    
    std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> dfsUris;
    std::map<std::string, uint32_t> permissionUris;
    
    PreProcessUtils preProcessUtils;
    preProcessUtils.FillUris(data, dfsUris, permissionUris);
    EXPECT_TRUE(dfsUris.empty());
}

/**
 * @tc.name: CheckUriAuthorization003
 * @tc.desc: Test CheckUriAuthorization with valid URIs
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, CheckUriAuthorization003, TestSize.Level1)
{
    const std::vector<std::string> uris = {"file://test.com/data.txt"};
    uint32_t tokenId = 100;
    std::map<std::string, uint32_t> permissionUris;
    
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.CheckUriAuthorization(uris, tokenId, permissionUris, false);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ProcessHtmlEntryAuthorization001
 * @tc.desc: Normal test of ProcessHtmlEntryAuthorization
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, ProcessHtmlEntryAuthorization001, TestSize.Level1)
{
    std::string html = "<img data-ohos='clipboard' src='file:///data/storage/el2/base/haps/102.png'>"
                        "<img data-ohos='clipboard' src='file:///data/storage/el2/base/haps/103.png'>";
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_["uniformDataType"] = "general.html";
    obj->value_["htmlContent"] = html;
    obj->value_["plainContent"] = "htmlPlainContent";
    auto record = std::make_shared<UnifiedRecord>(UDType::HTML, obj);
    bool isLocal = true;
    int32_t permissionPolicyMode = 1;
    std::map<std::string, uint32_t> strUris;
    
    PreProcessUtils preProcessUtils;
    preProcessUtils.ProcessHtmlEntryAuthorization(record, isLocal, permissionPolicyMode, strUris);
    EXPECT_TRUE(strUris.empty());
}

/**
 * @tc.name: ProcessFileAuthorization001
 * @tc.desc: Normal test of ProcessFileAuthorization
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UdmfPreProcessUtilsTest, ProcessFileAuthorization001, TestSize.Level1)
{
    UnifiedData data;
    auto record = nullptr;
    data.AddRecord(record);
    bool hasError = true;
    bool isLocal = true;
    std::map<std::string, unsigned int> uriPermissions;
    
    PreProcessUtils preProcessUtils;
    preProcessUtils.ProcessFileAuthorization(hasError, data, isLocal, uriPermissions);
    EXPECT_TRUE(hasError);
}

/**
* @tc.name: SetRemoteData005
* @tc.desc: Normal test of SetRemoteData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsTest, SetRemoteData005, TestSize.Level1)
{
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    std::shared_ptr<Object> detailObj = nullptr;
    obj->value_[DETAILS] = detailObj;
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data.AddRecord(record);
    Runtime runtime;
    runtime.deviceId = PreProcessUtils::GetLocalDeviceId();
    data.SetRuntime(runtime);
    PreProcessUtils::SetRemoteData(data);
    auto detailGet = std::get<std::shared_ptr<Object>>(obj->value_[DETAILS]);
    EXPECT_TRUE(detailGet == nullptr);
}
}
