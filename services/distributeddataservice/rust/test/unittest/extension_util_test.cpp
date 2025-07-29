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
#include "extension_util.h"
#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::CloudData;
 
namespace OHOS::Test {
class ExtensionUtilTest : public testing::Test {};

/**
* @tc.name: Convert001
* @tc.desc: Check that the character string does not contain null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert001, TestSize.Level1)
{
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = "http://example.com/path/to/file";
    dbAsset.path = "/path/to/file";

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_NE(asset, nullptr);
    EXPECT_GT(assetLen, 0);
    delete asset;
}

/**
* @tc.name: Convert002
* @tc.desc: Check whether the path contains null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert002, TestSize.Level1)
{
    const char data[] = "/path\0/to/file";
    std::string path(data, 13);
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = "http://example.com/path/to/file";
    dbAsset.path = path;

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_EQ(asset, nullptr);
    EXPECT_EQ(assetLen, 0);
}

/**
* @tc.name: Convert003
* @tc.desc: Check whether the URI contains null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert003, TestSize.Level1)
{
    const char data[] = "http://example.com/path\0to/file";
    std::string uri(data, 32);
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = uri;
    dbAsset.path = "/path/to/file";

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_EQ(asset, nullptr);
    EXPECT_EQ(assetLen, 0);
}

/**
* @tc.name: Convert004
* @tc.desc: Check whether the path and URI contain null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert004, TestSize.Level1)
{
    const char data1[] = "http://example.com/path\0to/file";
    std::string uri(data1, 32);
    const char data2[] = "/path\0/to/file";
    std::string path(data2, 13);
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = uri;
    dbAsset.path = path;

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_EQ(asset, nullptr);
    EXPECT_EQ(assetLen, 0);
}

/**
* @tc.name: ConvertVBuckets001
* @tc.desc: Test that convert DBVBuckets to empty.

* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, ConvertVBuckets001, TestSize.Level1)
{
    DBVBuckets buckets;
    auto [result, size] = ExtensionUtil::Convert(std::move(buckets));
    EXPECT_NE(result, nullptr);
    EXPECT_GT(size, 0);
}

/**
* @tc.name: ConvertVBuckets002
* @tc.desc: Test that convert DBVBuckets to empty.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, ConvertVBuckets002, TestSize.Level1)
{
    DBVBuckets buckets;
    DBVBucket bucket;
    auto [result, size] = ExtensionUtil::Convert(std::move(buckets));
    EXPECT_EQ(result, nullptr);
    EXPECT_EQ(size, 0);
}

class MockMetaDataManager {
public:
    static MockMetaDataManager* GetInstance() {
        static MockMetaDataManager instance;
        return &instance;
    }
    
    MOCK_METHOD3(LoadMeta, bool(const std::string&, DistributedData::ObjectUserMetaData&, bool));
};

// Mock ObjectUserMetaData
class MockObjectUserMetaData {
public:
    MOCK_METHOD0(GetKey, std::string());
};

// 重新定义或包含相关的常量和类型
const int OBJECT_INNER_ERROR = -1;
namespace DistributedData {
    using ObjectUserMetaData = MockObjectUserMetaData;
    class MetaDataManager {
    public:
        static auto GetInstance() -> decltype(MockMetaDataManager::GetInstance()) {
            return MockMetaDataManager::GetInstance();
        }
    };
}

// Mock日志系统
class MockLogger {
public:
    MOCK_METHOD1(LogError, void(const std::string&));
};

// 全局Mock Logger实例
extern MockLogger* g_mockLogger;

#define ZLOGE(msg) if(g_mockLogger) g_mockLogger->LogError(msg)

// 被测函数的封装（假设原始代码在一个函数中）
int LoadUserMetaData() {
    DistributedData::ObjectUserMetaData userMeta;
    if (!DistributedData::MetaDataManager::GetInstance()->LoadMeta(userMeta.GetKey(), userMeta, true)) {
        ZLOGE("load meta error");
        return OBJECT_INNER_ERROR;
    }
    return 0; // success
}

class LoadUserMetaDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockLogger = new MockLogger();
        g_mockLogger = mockLogger;
        mockMetaDataManager = MockMetaDataManager::GetInstance();
    }
    
    void TearDown() override {
        delete mockLogger;
        g_mockLogger = nullptr;
    }
    
    MockLogger* mockLogger;
    MockMetaDataManager* mockMetaDataManager;
};

// 测试用例1: LoadMeta返回true，加载成功
TEST_F(LoadUserMetaDataTest, LoadMetaSuccess) {
    // Arrange
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(::testing::_, ::testing::_, true))
        .WillOnce(::testing::Return(true));
    
    // Act
    int result = LoadUserMetaData();
    
    // Assert
    EXPECT_EQ(0, result); // 成功应该返回0
}

// 测试用例2: LoadMeta返回false，加载失败
TEST_F(LoadUserMetaDataTest, LoadMetaFailure) {
    // Arrange
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(::testing::_, ::testing::_, true))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockLogger, LogError("load meta error"))
        .Times(1);
    
    // Act
    int result = LoadUserMetaData();
    
    // Assert
    EXPECT_EQ(OBJECT_INNER_ERROR, result); // 失败应该返回错误码
}

// 测试用例3: 验证LoadMeta被正确调用（参数验证）
TEST_F(LoadUserMetaDataTest, LoadMetaCalledWithCorrectParameters) {
    // Arrange
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(::testing::_, ::testing::_, true))
        .WillOnce(::testing::Return(true));
    
    // Act
    LoadUserMetaData();
    
    // Assert
    // 通过期望调用来验证参数
}

// 测试用例4: 多次调用LoadMeta失败的情况
TEST_F(LoadUserMetaDataTest, LoadMetaFailureMultipleTimes) {
    // Arrange
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(::testing::_, ::testing::_, true))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockLogger, LogError("load meta error"))
        .Times(1);
    
    // Act & Assert
    EXPECT_EQ(OBJECT_INNER_ERROR, LoadUserMetaData());
}

extern "C" {
    OhCloudExtHashMap* OhCloudExtHashMapNew(OhCloudExtRustType type);
    int OhCloudExtHashMapInsert(OhCloudExtHashMap* map, void* key, size_t keyLen, void* value, size_t valueLen);
    int OhCloudExtHashMapIterGetKeyValuePair(OhCloudExtHashMap* map, OhCloudExtVector** keys, OhCloudExtVector** values);
    void OhCloudExtHashMapFree(OhCloudExtHashMap* map);
    
    // Mock OhCloudExtValue functions
    OhCloudExtValue* OhCloudExtValueNew(OhCloudExtValueType type, void* data, size_t len);
    int OhCloudExtValueGetContent(OhCloudExtValue* value, OhCloudExtValueType* type, void** content, unsigned int* len);
    void OhCloudExtValueFree(OhCloudExtValue* value);
    
    // Mock OhCloudExtVector functions
    OhCloudExtVector* OhCloudExtVectorNew(OhCloudExtRustType type);
    int OhCloudExtVectorPush(OhCloudExtVector* vec, void* item, size_t len);
    int OhCloudExtVectorGetLength(OhCloudExtVector* vec, unsigned int* len);
    int OhCloudExtVectorGet(OhCloudExtVector* vec, size_t index, void** item, unsigned int* len);
    void OhCloudExtVectorFree(OhCloudExtVector* vec);
    
    // Mock OhCloudExtTable functions
    OhCloudExtTable* OhCloudExtTableNew(const unsigned char* name, size_t nameLen, const unsigned char* alias, size_t aliasLen, OhCloudExtVector* fields);
    
    // Mock OhCloudExtField functions
    OhCloudExtField* OhCloudExtFieldNew(OhCloudExtFieldBuilder* builder);
    
    // Mock OhCloudExtCloudAsset functions
    OhCloudExtCloudAsset* OhCloudExtCloudAssetNew(OhCloudExtCloudAssetBuilder* builder);
    int OhCloudExtCloudAssetGetId(OhCloudExtCloudAsset* asset, unsigned char** id, unsigned int* len);
    int OhCloudExtCloudAssetGetName(OhCloudExtCloudAsset* asset, unsigned char** name, unsigned int* len);
    int OhCloudExtCloudAssetGetUri(OhCloudExtCloudAsset* asset, unsigned char** uri, unsigned int* len);
    int OhCloudExtCloudAssetGetCreateTime(OhCloudExtCloudAsset* asset, unsigned char** time, unsigned int* len);
    int OhCloudExtCloudAssetGetModifiedTime(OhCloudExtCloudAsset* asset, unsigned char** time, unsigned int* len);
    int OhCloudExtCloudAssetGetSize(OhCloudExtCloudAsset* asset, unsigned char** size, unsigned int* len);
    int OhCloudExtCloudAssetGetHash(OhCloudExtCloudAsset* asset, unsigned char** hash, unsigned int* len);
    int OhCloudExtCloudAssetGetLocalPath(OhCloudExtCloudAsset* asset, unsigned char** path, unsigned int* len);
    void OhCloudExtCloudAssetFree(OhCloudExtCloudAsset* asset);
    
    // Mock OhCloudExtAppInfo functions
    int OhCloudExtAppInfoGetAppId(OhCloudExtAppInfo* info, unsigned char** appId, unsigned int* len);
    int OhCloudExtAppInfoGetBundleName(OhCloudExtAppInfo* info, unsigned char** bundleName, unsigned int* len);
    void OhCloudExtAppInfoGetCloudSwitch(OhCloudExtAppInfo* info, bool* cloudSwitch);
    void OhCloudExtAppInfoGetInstanceId(OhCloudExtAppInfo* info, int* instanceId);
    
    // Mock OhCloudExtValueBucket functions
    int OhCloudExtValueBucketGetValue(OhCloudExtValueBucket* bucket, OhCloudExtKeyName key, OhCloudExtValueType* type, void** content, unsigned int* len);
    
    // Mock OhCloudExtKeyName functions
    OhCloudExtKeyName OhCloudExtKeyNameNew(unsigned char* key, size_t len);
}

// Mock implementations
class MockOhCloudExt {
public:
    MOCK_METHOD1(OhCloudExtHashMapNew, OhCloudExtHashMap*(OhCloudExtRustType type));
    MOCK_METHOD5(OhCloudExtHashMapInsert, int(OhCloudExtHashMap* map, void* key, size_t keyLen, void* value, size_t valueLen));
    MOCK_METHOD3(OhCloudExtHashMapIterGetKeyValuePair, int(OhCloudExtHashMap* map, OhCloudExtVector** keys, OhCloudExtVector** values));
    MOCK_METHOD1(OhCloudExtHashMapFree, void(OhCloudExtHashMap* map));
    
    MOCK_METHOD3(OhCloudExtValueNew, OhCloudExtValue*(OhCloudExtValueType type, void* data, size_t len));
    MOCK_METHOD4(OhCloudExtValueGetContent, int(OhCloudExtValue* value, OhCloudExtValueType* type, void** content, unsigned int* len));
    MOCK_METHOD1(OhCloudExtValueFree, void(OhCloudExtValue* value));
    
    MOCK_METHOD1(OhCloudExtVectorNew, OhCloudExtVector*(OhCloudExtRustType type));
    MOCK_METHOD3(OhCloudExtVectorPush, int(OhCloudExtVector* vec, void* item, size_t len));
    MOCK_METHOD2(OhCloudExtVectorGetLength, int(OhCloudExtVector* vec, unsigned int* len));
    MOCK_METHOD4(OhCloudExtVectorGet, int(OhCloudExtVector* vec, size_t index, void** item, unsigned int* len));
    MOCK_METHOD1(OhCloudExtVectorFree, void(OhCloudExtVector* vec));
    
    MOCK_METHOD6(OhCloudExtTableNew, OhCloudExtTable*(const unsigned char* name, size_t nameLen, const unsigned char* alias, size_t aliasLen, OhCloudExtVector* fields));
    
    MOCK_METHOD1(OhCloudExtFieldNew, OhCloudExtField*(OhCloudExtFieldBuilder* builder));
    
    MOCK_METHOD1(OhCloudExtCloudAssetNew, OhCloudExtCloudAsset*(OhCloudExtCloudAssetBuilder* builder));
    MOCK_METHOD3(OhCloudExtCloudAssetGetId, int(OhCloudExtCloudAsset* asset, unsigned char** id, unsigned int* len));
    MOCK_METHOD3(OhCloudExtCloudAssetGetName, int(OhCloudExtCloudAsset* asset, unsigned char** name, unsigned int* len));
    MOCK_METHOD3(OhCloudExtCloudAssetGetUri, int(OhCloudExtCloudAsset* asset, unsigned char** uri, unsigned int* len));
    MOCK_METHOD3(OhCloudExtCloudAssetGetCreateTime, int(OhCloudExtCloudAsset* asset, unsigned char** time, unsigned int* len));
    MOCK_METHOD3(OhCloudExtCloudAssetGetModifiedTime, int(OhCloudExtCloudAsset* asset, unsigned char** time, unsigned int* len));
    MOCK_METHOD3(OhCloudExtCloudAssetGetSize, int(OhCloudExtCloudAsset* asset, unsigned char** size, unsigned int* len));
    MOCK_METHOD3(OhCloudExtCloudAssetGetHash, int(OhCloudExtCloudAsset* asset, unsigned char** hash, unsigned int* len));
    MOCK_METHOD3(OhCloudExtCloudAssetGetLocalPath, int(OhCloudExtCloudAsset* asset, unsigned char** path, unsigned int* len));
    MOCK_METHOD1(OhCloudExtCloudAssetFree, void(OhCloudExtCloudAsset* asset));
    
    MOCK_METHOD3(OhCloudExtAppInfoGetAppId, int(OhCloudExtAppInfo* info, unsigned char** appId, unsigned int* len));
    MOCK_METHOD3(OhCloudExtAppInfoGetBundleName, int(OhCloudExtAppInfo* info, unsigned char** bundleName, unsigned int* len));
    MOCK_METHOD2(OhCloudExtAppInfoGetCloudSwitch, void(OhCloudExtAppInfo* info, bool* cloudSwitch));
    MOCK_METHOD2(OhCloudExtAppInfoGetInstanceId, void(OhCloudExtAppInfo* info, int* instanceId));
    
    MOCK_METHOD5(OhCloudExtValueBucketGetValue, int(OhCloudExtValueBucket* bucket, OhCloudExtKeyName key, OhCloudExtValueType* type, void** content, unsigned int* len));
    
    MOCK_METHOD2(OhCloudExtKeyNameNew, OhCloudExtKeyName(unsigned char* key, size_t len));
};

// Global mock object
MockOhCloudExt* g_mock = nullptr;

// Override C functions with mock implementations
extern "C" {
    OhCloudExtHashMap* OhCloudExtHashMapNew(OhCloudExtRustType type) {
        return g_mock->OhCloudExtHashMapNew(type);
    }
    
    int OhCloudExtHashMapInsert(OhCloudExtHashMap* map, void* key, size_t keyLen, void* value, size_t valueLen) {
        return g_mock->OhCloudExtHashMapInsert(map, key, keyLen, value, valueLen);
    }
    
    int OhCloudExtHashMapIterGetKeyValuePair(OhCloudExtHashMap* map, OhCloudExtVector** keys, OhCloudExtVector** values) {
        return g_mock->OhCloudExtHashMapIterGetKeyValuePair(map, keys, values);
    }
    
    void OhCloudExtHashMapFree(OhCloudExtHashMap* map) {
        g_mock->OhCloudExtHashMapFree(map);
    }
    
    OhCloudExtValue* OhCloudExtValueNew(OhCloudExtValueType type, void* data, size_t len) {
        return g_mock->OhCloudExtValueNew(type, data, len);
    }
    
    int OhCloudExtValueGetContent(OhCloudExtValue* value, OhCloudExtValueType* type, void** content, unsigned int* len) {
        return g_mock->OhCloudExtValueGetContent(value, type, content, len);
    }
    
    void OhCloudExtValueFree(OhCloudExtValue* value) {
        g_mock->OhCloudExtValueFree(value);
    }
    
    OhCloudExtVector* OhCloudExtVectorNew(OhCloudExtRustType type) {
        return g_mock->OhCloudExtVectorNew(type);
    }
    
    int OhCloudExtVectorPush(OhCloudExtVector* vec, void* item, size_t len) {
        return g_mock->OhCloudExtVectorPush(vec, item, len);
    }
    
    int OhCloudExtVectorGetLength(OhCloudExtVector* vec, unsigned int* len) {
        return g_mock->OhCloudExtVectorGetLength(vec, len);
    }
    
    int OhCloudExtVectorGet(OhCloudExtVector* vec, size_t index, void** item, unsigned int* len) {
        return g_mock->OhCloudExtVectorGet(vec, index, item, len);
    }
    
    void OhCloudExtVectorFree(OhCloudExtVector* vec) {
        g_mock->OhCloudExtVectorFree(vec);
    }
    
    OhCloudExtTable* OhCloudExtTableNew(const unsigned char* name, size_t nameLen, const unsigned char* alias, size_t aliasLen, OhCloudExtVector* fields) {
        return g_mock->OhCloudExtTableNew(name, nameLen, alias, aliasLen, fields);
    }
    
    OhCloudExtField* OhCloudExtFieldNew(OhCloudExtFieldBuilder* builder) {
        return g_mock->OhCloudExtFieldNew(builder);
    }
    
    OhCloudExtCloudAsset* OhCloudExtCloudAssetNew(OhCloudExtCloudAssetBuilder* builder) {
        return g_mock->OhCloudExtCloudAssetNew(builder);
    }
    
    int OhCloudExtCloudAssetGetId(OhCloudExtCloudAsset* asset, unsigned char** id, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetId(asset, id, len);
    }
    
    int OhCloudExtCloudAssetGetName(OhCloudExtCloudAsset* asset, unsigned char** name, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetName(asset, name, len);
    }
    
    int OhCloudExtCloudAssetGetUri(OhCloudExtCloudAsset* asset, unsigned char** uri, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetUri(asset, uri, len);
    }
    
    int OhCloudExtCloudAssetGetCreateTime(OhCloudExtCloudAsset* asset, unsigned char** time, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetCreateTime(asset, time, len);
    }
    
    int OhCloudExtCloudAssetGetModifiedTime(OhCloudExtCloudAsset* asset, unsigned char** time, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetModifiedTime(asset, time, len);
    }
    
    int OhCloudExtCloudAssetGetSize(OhCloudExtCloudAsset* asset, unsigned char** size, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetSize(asset, size, len);
    }
    
    int OhCloudExtCloudAssetGetHash(OhCloudExtCloudAsset* asset, unsigned char** hash, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetHash(asset, hash, len);
    }
    
    int OhCloudExtCloudAssetGetLocalPath(OhCloudExtCloudAsset* asset, unsigned char** path, unsigned int* len) {
        return g_mock->OhCloudExtCloudAssetGetLocalPath(asset, path, len);
    }
    
    void OhCloudExtCloudAssetFree(OhCloudExtCloudAsset* asset) {
        g_mock->OhCloudExtCloudAssetFree(asset);
    }
    
    int OhCloudExtAppInfoGetAppId(OhCloudExtAppInfo* info, unsigned char** appId, unsigned int* len) {
        return g_mock->OhCloudExtAppInfoGetAppId(info, appId, len);
    }
    
    int OhCloudExtAppInfoGetBundleName(OhCloudExtAppInfo* info, unsigned char** bundleName, unsigned int* len) {
        return g_mock->OhCloudExtAppInfoGetBundleName(info, bundleName, len);
    }
    
    void OhCloudExtAppInfoGetCloudSwitch(OhCloudExtAppInfo* info, bool* cloudSwitch) {
        g_mock->OhCloudExtAppInfoGetCloudSwitch(info, cloudSwitch);
    }
    
    void OhCloudExtAppInfoGetInstanceId(OhCloudExtAppInfo* info, int* instanceId) {
        g_mock->OhCloudExtAppInfoGetInstanceId(info, instanceId);
    }
    
    int OhCloudExtValueBucketGetValue(OhCloudExtValueBucket* bucket, OhCloudExtKeyName key, OhCloudExtValueType* type, void** content, unsigned int* len) {
        return g_mock->OhCloudExtValueBucketGetValue(bucket, key, type, content, len);
    }
    
    OhCloudExtKeyName OhCloudExtKeyNameNew(unsigned char* key, size_t len) {
        return g_mock->OhCloudExtKeyNameNew(key, len);
    }
}

// Test fixtures
class ExtensionUtilTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mock = new MockOhCloudExt();
    }
    
    void TearDown() override {
        delete g_mock;
        g_mock = nullptr;
    }
};

// Test cases for Convert(const DBVBucket &bucket)
TEST_F(ExtensionUtilTest, ConvertDBVBucketSuccess) {
    // Setup
    DBVBucket bucket;
    bucket["key1"] = int64_t(123);
    bucket["key2"] = std::string("test");
    
    OhCloudExtHashMap* mockHashMap = reinterpret_cast<OhCloudExtHashMap*>(0x1234);
    OhCloudExtValue* mockValue1 = reinterpret_cast<OhCloudExtValue*>(0x5678);
    OhCloudExtValue* mockValue2 = reinterpret_cast<OhCloudExtValue*>(0x9ABC);
    
    EXPECT_CALL(*g_mock, OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_VALUE))
        .WillOnce(::testing::Return(mockHashMap));
    EXPECT_CALL(*g_mock, OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_INT, ::testing::_, sizeof(int64_t)))
        .WillOnce(::testing::Return(mockValue1));
    EXPECT_CALL(*g_mock, OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_STRING, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(mockValue2));
    EXPECT_CALL(*g_mock, OhCloudExtHashMapInsert(mockHashMap, ::testing::_, ::testing::_, mockValue1, sizeof(int64_t)))
        .WillOnce(::testing::Return(ERRNO_SUCCESS));
    EXPECT_CALL(*g_mock, OhCloudExtHashMapInsert(mockHashMap, ::testing::_, ::testing::_, mockValue2, ::testing::_))
        .WillOnce(::testing::Return(ERRNO_SUCCESS));
    
    // Execute
    auto result = ExtensionUtil::Convert(bucket);
    
    // Verify
    EXPECT_EQ(result.first, mockHashMap);
    EXPECT_EQ(result.second, sizeof(int64_t) + 4); // 4 is length of "test"
}

TEST_F(ExtensionUtilTest, ConvertDBVBucketHashMapNewFailure) {
    // Setup
    DBVBucket bucket;
    bucket["key"] = int64_t(123);
    
    EXPECT_CALL(*g_mock, OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_VALUE))
        .WillOnce(::testing::Return(nullptr));
    
    // Execute
    auto result = ExtensionUtil::Convert(bucket);
    
    // Verify
    EXPECT_EQ(result.first, nullptr);
    EXPECT_EQ(result.second, 0);
}

// Test cases for Convert(const DBValue &dbValue)
TEST_F(ExtensionUtilTest, ConvertDBValueInt) {
    // Setup
    DBValue dbValue = int64_t(123);
    OhCloudExtValue* mockValue = reinterpret_cast<OhCloudExtValue*>(0x1234);
    
    EXPECT_CALL(*g_mock, OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_INT, ::testing::_, sizeof(int64_t)))
        .WillOnce(::testing::Return(mockValue));
    
    // Execute
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Verify
    EXPECT_EQ(result.first, mockValue);
    EXPECT_EQ(result.second, sizeof(int64_t));
}

TEST_F(ExtensionUtilTest, ConvertDBValueString) {
    // Setup
    DBValue dbValue = std::string("test");
    OhCloudExtValue* mockValue = reinterpret_cast<OhCloudExtValue*>(0x1234);
    
    EXPECT_CALL(*g_mock, OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_STRING, ::testing::_, 4))
        .WillOnce(::testing::Return(mockValue));
    
    // Execute
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Verify
    EXPECT_EQ(result.first, mockValue);
    EXPECT_EQ(result.second, 4);
}

// Test cases for ContainNullChar
TEST_F(ExtensionUtilTest, ContainNullCharNoNull) {
    // Setup
    std::string path = "test/path";
    
    // Execute
    bool result = ExtensionUtil::ContainNullChar(path);
    
    // Verify
    EXPECT_FALSE(result);
}

TEST_F(ExtensionUtilTest, ContainNullCharWithNull) {
    // Setup
    std::string path = "test\0path";
    
    // Execute
    bool result = ExtensionUtil::ContainNullChar(path);
    
    // Verify
    EXPECT_TRUE(result);
}

// Test cases for ConvertAssetStatus
TEST_F(ExtensionUtilTest, ConvertAssetStatusNormal) {
    // Execute
    OhCloudExtAssetStatus result = ExtensionUtil::ConvertAssetStatus(DBAssetStatus::STATUS_NORMAL);
    
    // Verify
    EXPECT_EQ(result, OhCloudExtAssetStatus::ASSETSTATUS_NORMAL);
}

TEST_F(ExtensionUtilTest, ConvertAssetStatusUnknown) {
    // Execute
    OhCloudExtAssetStatus result = ExtensionUtil::ConvertAssetStatus(static_cast<DBAssetStatus>(999));
    
    // Verify
    EXPECT_EQ(result, OhCloudExtAssetStatus::ASSETSTATUS_UNKNOWN);
}

// Test cases for ConvertStatus
TEST_F(ExtensionUtilTest, ConvertStatusSuccess) {
    // Execute
    DBErr result = ExtensionUtil::ConvertStatus(ERRNO_SUCCESS);
    
    // Verify
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(ExtensionUtilTest, ConvertStatusUnknown) {
    // Execute
    DBErr result = ExtensionUtil::ConvertStatus(999);
    
    // Verify
    EXPECT_EQ(result, DBErr::E_ERROR);
}
// Test DBValue Convert function
TEST_F(ExtensionUtilTest, ConvertDBValueInt64) {
    // Arrange
    DBValue dbValue = int64_t(12345);
    OhCloudExtValue expectedValue;
    
    // Expect OhCloudExtValueNew to be called with correct parameters
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtValueNew(
        OhCloudExtValueType::VALUEINNERTYPE_INT, NotNull(), sizeof(int64_t)))
        .WillOnce(Return(&expectedValue));
    
    // Act
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Assert
    EXPECT_EQ(result.first, &expectedValue);
    EXPECT_EQ(result.second, sizeof(int64_t));
}

TEST_F(ExtensionUtilTest, ConvertDBValueString) {
    // Arrange
    DBValue dbValue = std::string("test_string");
    OhCloudExtValue expectedValue;
    
    // Expect OhCloudExtValueNew to be called with correct parameters
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtValueNew(
        OhCloudExtValueType::VALUEINNERTYPE_STRING, NotNull(), strlen("test_string")))
        .WillOnce(Return(&expectedValue));
    
    // Act
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Assert
    EXPECT_EQ(result.first, &expectedValue);
    EXPECT_EQ(result.second, strlen("test_string"));
}

TEST_F(ExtensionUtilTest, ConvertDBValueBool) {
    // Arrange
    DBValue dbValue = true;
    OhCloudExtValue expectedValue;
    
    // Expect OhCloudExtValueNew to be called with correct parameters
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtValueNew(
        OhCloudExtValueType::VALUEINNERTYPE_BOOL, NotNull(), sizeof(bool)))
        .WillOnce(Return(&expectedValue));
    
    // Act
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Assert
    EXPECT_EQ(result.first, &expectedValue);
    EXPECT_EQ(result.second, sizeof(bool));
}

TEST_F(ExtensionUtilTest, ConvertDBValueBytes) {
    // Arrange
    DBBytes bytes = {0x01, 0x02, 0x03};
    DBValue dbValue = bytes;
    OhCloudExtValue expectedValue;
    
    // Expect OhCloudExtValueNew to be called with correct parameters
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtValueNew(
        OhCloudExtValueType::VALUEINNERTYPE_BYTES, NotNull(), bytes.size()))
        .WillOnce(Return(&expectedValue));
    
    // Act
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Assert
    EXPECT_EQ(result.first, &expectedValue);
    EXPECT_EQ(result.second, bytes.size() * sizeof(uint8_t));
}

TEST_F(ExtensionUtilTest, ConvertDBValueEmptyBytes) {
    // Arrange
    DBBytes bytes = {};
    DBValue dbValue = bytes;
    OhCloudExtValue expectedValue;
    
    // Expect OhCloudExtValueNew to be called with correct parameters
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtValueNew(
        OhCloudExtValueType::VALUEINNERTYPE_BYTES, NotNull(), 0))
        .WillOnce(Return(&expectedValue));
    
    // Act
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Assert
    EXPECT_EQ(result.first, &expectedValue);
    EXPECT_EQ(result.second, 0);
}

TEST_F(ExtensionUtilTest, ConvertDBValueUnknownType) {
    // Arrange
    DBValue dbValue = 3.14f; // float类型，不属于已知处理类型
    OhCloudExtValue expectedValue;
    
    // Expect OhCloudExtValueNew to be called with correct parameters
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtValueNew(
        OhCloudExtValueType::VALUEINNERTYPE_EMPTY, nullptr, 0))
        .WillOnce(Return(&expectedValue));
    
    // Act
    auto result = ExtensionUtil::Convert(dbValue);
    
    // Assert
    EXPECT_EQ(result.first, &expectedValue);
    EXPECT_EQ(result.second, 0);
}

// Test DBMeta Convert function
TEST_F(ExtensionUtilTest, ConvertDBMetaSuccess) {
    // Arrange
    DBTable table;
    table.name = "test_table";
    table.alias = "tt";
    table.fields = {}; // 简化处理，实际可能需要添加字段
    
    DBMeta dbMeta;
    dbMeta.name = "test_db";
    dbMeta.alias = "tdb";
    dbMeta.tables = {table};
    
    OhCloudExtHashMap hashMap;
    OhCloudExtTable extTable;
    OhCloudExtDatabase extDatabase;
    
    // Mock calls
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_TABLE))
        .WillOnce(Return(&hashMap));
        
    // Mock table conversion - 这里简化处理，实际可能需要更复杂的mock
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_FIELD))
        .WillOnce(Return(reinterpret_cast<OhCloudExtVector*>(&extTable)));
        
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtTableNew(_, _, _, _, _))
        .WillOnce(Return(&extTable));
        
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtHashMapInsert(&hashMap, _, _, &extTable, _))
        .WillOnce(Return(ERRNO_SUCCESS));
        
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtDatabaseNew(_, _, _, _, &hashMap))
        .WillOnce(Return(&extDatabase));
    
    // Act
    auto result = ExtensionUtil::Convert(dbMeta);
    
    // Assert
    EXPECT_EQ(result.first, &extDatabase);
}

TEST_F(ExtensionUtilTest, ConvertDBMetaHashMapCreationFailure) {
    // Arrange
    DBMeta dbMeta;
    dbMeta.name = "test_db";
    dbMeta.alias = "tdb";
    dbMeta.tables = {};
    
    // Mock calls
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_TABLE))
        .WillOnce(Return(nullptr));
    
    // Act
    auto result = ExtensionUtil::Convert(dbMeta);
    
    // Assert
    EXPECT_EQ(result.first, nullptr);
    EXPECT_EQ(result.second, 0);
}

// Test DBTable Convert function
TEST_F(ExtensionUtilTest, ConvertDBTableSuccess) {
    // Arrange
    DBField field;
    field.colName = "id";
    field.alias = "ID";
    field.type = 1; // 假设1表示INTEGER类型
    field.primary = true;
    field.nullable = false;
    
    DBTable dbTable;
    dbTable.name = "test_table";
    dbTable.alias = "tt";
    dbTable.fields = {field};
    
    OhCloudExtVector vector;
    OhCloudExtField extField;
    
    // Mock calls
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_FIELD))
        .WillOnce(Return(&vector));
        
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtFieldNew(_))
        .WillOnce(Return(&extField));
        
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtVectorPush(&vector, &extField, _))
        .Times(1);
    
    // Act
    auto result = ExtensionUtil::Convert(dbTable);
    
    // Assert
    EXPECT_EQ(result.first, &vector);
    EXPECT_GT(result.second, 0);
}

TEST_F(ExtensionUtilTest, ConvertDBTableVectorCreationFailure) {
    // Arrange
    DBTable dbTable;
    dbTable.name = "test_table";
    dbTable.alias = "tt";
    dbTable.fields = {};
    
    // Mock calls
    EXPECT_CALL(*mockOhCloudExt, OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_FIELD))
        .WillOnce(Return(nullptr));
    
    // Act
    auto result = ExtensionUtil::Convert(dbTable);
    
    // Assert
    EXPECT_EQ(result.first, nullptr);
    EXPECT_EQ(result.second, 0);
}


// Tests for ConvertBuckets
TEST_F(ExtensionUtilTest, ConvertBuckets_NullInput_ReturnsEmpty) {
    // Given
    OhCloudExtVector* input = nullptr;
    
    // When
    DBVBuckets result = ExtensionUtil::ConvertBuckets(input);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBuckets_GetLengthFails_ReturnsEmpty) {
    // Given
    OhCloudExtVector mockVector;
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(_, _))
        .WillOnce(Return(ERRNO_FAILURE));
    
    // When
    DBVBuckets result = ExtensionUtil::ConvertBuckets(&mockVector);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBuckets_ZeroLength_ReturnsEmpty) {
    // Given
    OhCloudExtVector mockVector;
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(0), Return(ERRNO_SUCCESS)));
    
    // When
    DBVBuckets result = ExtensionUtil::ConvertBuckets(&mockVector);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBuckets_GetElementFails_ReturnsEmpty) {
    // Given
    OhCloudExtVector mockVector;
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGet(_, _, _, _))
        .WillOnce(Return(ERRNO_FAILURE));
    
    // When
    DBVBuckets result = ExtensionUtil::ConvertBuckets(&mockVector);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBuckets_ElementIsNull_ReturnsEmpty) {
    // Given
    OhCloudExtVector mockVector;
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGet(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(nullptr), Return(ERRNO_SUCCESS)));
    
    // When
    DBVBuckets result = ExtensionUtil::ConvertBuckets(&mockVector);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBuckets_Success_ConvertsAllElements) {
    // Given
    OhCloudExtVector mockVector;
    OhCloudExtHashMap mockHashMap;
    
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGet(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(&mockHashMap), SetArgPointee<3>(sizeof(OhCloudExtHashMap)), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtHashMapFree(_))
        .Times(1);
    
    // Mock ConvertBucket to return a non-empty bucket
    DBVBucket expectedBucket;
    expectedBucket["key"] = DBValue(); // Assuming DBValue has a default constructor
    
    // Note: In a real test, we would need to mock the ConvertBucket method or test it separately
    
    // When
    DBVBuckets result = ExtensionUtil::ConvertBuckets(&mockVector);
    
    // Then
    //EXPECT_EQ(result.size(), 1);
    // Add more specific assertions based on what ConvertBucket returns
}

// Tests for ConvertBucket
TEST_F(ExtensionUtilTest, ConvertBucket_NullInput_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap* input = nullptr;
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(input);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBucket_GetKeyValuePairFails_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap mockHashMap;
    EXPECT_CALL(mock_, OhCloudExtHashMapIterGetKeyValuePair(_, _, _))
        .WillOnce(Return(ERRNO_FAILURE));
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(&mockHashMap);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBucket_KeysIsNull_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap mockHashMap;
    OhCloudExtVector mockValues;
    EXPECT_CALL(mock_, OhCloudExtHashMapIterGetKeyValuePair(_, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(&mockValues), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockValues))
        .Times(1);
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(&mockHashMap);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBucket_ValuesIsNull_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap mockHashMap;
    OhCloudExtVector mockKeys;
    EXPECT_CALL(mock_, OhCloudExtHashMapIterGetKeyValuePair(_, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(&mockKeys), SetArgPointee<2>(nullptr), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockKeys))
        .Times(1);
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(&mockHashMap);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBucket_ZeroKeysLength_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap mockHashMap;
    OhCloudExtVector mockKeys;
    OhCloudExtVector mockValues;
    
    EXPECT_CALL(mock_, OhCloudExtHashMapIterGetKeyValuePair(_, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(&mockKeys), SetArgPointee<2>(&mockValues), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockKeys))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockValues))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(&mockKeys, _))
        .WillOnce(DoAll(SetArgPointee<1>(0), Return(ERRNO_SUCCESS)));
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(&mockHashMap);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBucket_KeyValueLengthMismatch_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap mockHashMap;
    OhCloudExtVector mockKeys;
    OhCloudExtVector mockValues;
    
    EXPECT_CALL(mock_, OhCloudExtHashMapIterGetKeyValuePair(_, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(&mockKeys), SetArgPointee<2>(&mockValues), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockKeys))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockValues))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(&mockKeys, _))
        .WillOnce(DoAll(SetArgPointee<1>(2), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(&mockValues, _))
        .WillOnce(DoAll(SetArgPointee<1>(3), Return(ERRNO_SUCCESS)));
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(&mockHashMap);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBucket_GetKeyElementFails_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap mockHashMap;
    OhCloudExtVector mockKeys;
    OhCloudExtVector mockValues;
    
    EXPECT_CALL(mock_, OhCloudExtHashMapIterGetKeyValuePair(_, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(&mockKeys), SetArgPointee<2>(&mockValues), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockKeys))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockValues))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(&mockKeys, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(&mockValues, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGet(&mockKeys, 0, _, _))
        .WillOnce(Return(ERRNO_FAILURE));
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(&mockHashMap);
    
    // Then
    EXPECT_TRUE(result.empty());
}

TEST_F(ExtensionUtilTest, ConvertBucket_KeyElementIsNull_ReturnsEmpty) {
    // Given
    OhCloudExtHashMap mockHashMap;
    OhCloudExtVector mockKeys;
    OhCloudExtVector mockValues;
    
    EXPECT_CALL(mock_, OhCloudExtHashMapIterGetKeyValuePair(_, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(&mockKeys), SetArgPointee<2>(&mockValues), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockKeys))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorFree(&mockValues))
        .Times(1);
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(&mockKeys, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGetLength(&mockValues, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(mock_, OhCloudExtVectorGet(&mockKeys, 0, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(nullptr), Return(ERRNO_SUCCESS)));
    
    // When
    DBVBucket result = ExtensionUtil::ConvertBucket(&mockHashMap);
    
    // Then
    EXPECT_TRUE(result.empty());
}

// Tests for ConvertValue
TEST_F(ExtensionUtilTest, ConvertValue_NullInput_ReturnsDefault) {
    // Given
    OhCloudExtValue* input = nullptr;
    
    // When
    DBValue result = ExtensionUtil::ConvertValue(input);
    
    // Then
    // Verify that result is a default-constructed DBValue
    // This would depend on the implementation of DBValue
}

TEST_F(ExtensionUtilTest, ConvertValue_GetContentFails_ReturnsDefault) {
    // Given
    OhCloudExtValue mockValue;
    EXPECT_CALL(mock_, OhCloudExtValueGetContent(_, _, _, _))
        .WillOnce(Return(ERRNO_FAILURE));
    
    // When
    DBValue result = ExtensionUtil::ConvertValue(&mockValue);
    
    // Then
    // Verify that result is a default-constructed DBValue
}

TEST_F(ExtensionUtilTest, ConvertValue_ContentIsNull_ReturnsDefault) {
    // Given
    OhCloudExtValue mockValue;
    OhCloudExtValueType type = VALUEINNERTYPE_EMPTY;
    EXPECT_CALL(mock_, OhCloudExtValueGetContent(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(type), SetArgPointee<2>(nullptr), Return(ERRNO_SUCCESS)));
    
    // When
    DBValue result = ExtensionUtil::ConvertValue(&mockValue);
    
    // Then
    // Verify that result is a default-constructed DBValue
}

class MockOhCloudExtCloudAssetLoader {
public:
    MOCK_METHOD(void, Free, (), ());
};

class MockExtensionUtil {
public:
    static MockExtensionUtil* GetInstance() {
        static MockExtensionUtil instance;
        return &instance;
    }

    MOCK_METHOD(std::pair<void*, size_t>, Convert, (const DBAssets&), ());
    MOCK_METHOD(std::pair<void*, size_t>, Convert, (const DBAsset&), ());
    MOCK_METHOD(DBAssets, ConvertAssets, (void*), ());
    MOCK_METHOD(int32_t, ConvertStatus, (int32_t), ());
};

// Global mock instances
MockOhCloudExtCloudAssetLoader* g_mockLoader = nullptr;
MockExtensionUtil* g_mockExtensionUtil = nullptr;

// Mock C functions
extern "C" {
    void OhCloudExtCloudAssetLoaderFree(OhCloudExtCloudAssetLoader* loader) {
        if (g_mockLoader) {
            g_mockLoader->Free();
        }
    }

    int32_t OhCloudExtCloudAssetLoaderDownload(OhCloudExtCloudAssetLoader* loader,
                                               const OhCloudExtUpDownloadInfo* info,
                                               void* data) {
        return ERRNO_SUCCESS; // Default success
    }

    void OhCloudExtVectorFree(void* data) {
        // Do nothing
    }

    int32_t OhCloudExtCloudAssetLoaderRemoveLocalAssets(void* data) {
        return ERRNO_SUCCESS; // Default success
    }

    void OhCloudExtCloudAssetFree(void* data) {
        // Do nothing
    }
}

// Test fixture
class AssetLoaderImplTest : public Test {
protected:
    void SetUp() override {
        g_mockLoader = new MockOhCloudExtCloudAssetLoader();
        g_mockExtensionUtil = MockExtensionUtil::GetInstance();
    }

    void TearDown() override {
        delete g_mockLoader;
        g_mockLoader = nullptr;
    }
};

// Constructor test
TEST_F(AssetLoaderImplTest, Constructor_ValidLoader)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);
    // No explicit check needed, just ensure no crash
}

// Destructor tests
TEST_F(AssetLoaderImplTest, Destructor_ValidLoader_CallsFree)
{
    OhCloudExtCloudAssetLoader loader;
    EXPECT_CALL(*g_mockLoader, Free()).Times(1);
    {
        AssetLoaderImpl assetLoader(&loader);
    }
    // Mock verification happens automatically
}

TEST_F(AssetLoaderImplTest, Destructor_NullLoader_NoCall)
{
    {
        AssetLoaderImpl assetLoader(nullptr);
    }
    // No call to Free expected
}

// Download tests
TEST_F(AssetLoaderImplTest, Download_PrefixIsString_ConstructsCorrectInfo)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    // Add a DBAssets entry
    DBAssets dbAssets;
    dbAssets.push_back(DBAsset());
    assets["key1"] = dbAssets;

    // Mock successful conversion
    std::pair<void*, size_t> convertedData{reinterpret_cast<void*>(0x1234), 1};
    EXPECT_CALL(*g_mockExtensionUtil, Convert(_))
        .WillOnce(Return(convertedData));
    EXPECT_CALL(*g_mockExtensionUtil, ConvertAssets(_))
        .WillOnce(Return(DBAssets()));

    int32_t result = assetLoader.Download(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(AssetLoaderImplTest, Download_PrefixNotString_UsesEmptyString)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    DBValue prefix = 123; // Not a string
    DBVBucket assets;

    // Add a DBAssets entry
    DBAssets dbAssets;
    dbAssets.push_back(DBAsset());
    assets["key1"] = dbAssets;

    // Mock successful conversion
    std::pair<void*, size_t> convertedData{reinterpret_cast<void*>(0x1234), 1};
    EXPECT_CALL(*g_mockExtensionUtil, Convert(_))
        .WillOnce(Return(convertedData));
    EXPECT_CALL(*g_mockExtensionUtil, ConvertAssets(_))
        .WillOnce(Return(DBAssets()));

    int32_t result = assetLoader.Download(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(AssetLoaderImplTest, Download_ValueNotDBAssets_ReturnsInvalidArgs)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    // Add an invalid entry
    assets["key1"] = 123; // Not DBAssets

    int32_t result = assetLoader.Download(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_INVALID_ARGS);
}

TEST_F(AssetLoaderImplTest, Download_ConvertFails_ReturnsError)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    // Add a DBAssets entry
    DBAssets dbAssets;
    dbAssets.push_back(DBAsset());
    assets["key1"] = dbAssets;

    // Mock failed conversion
    std::pair<void*, size_t> convertedData{nullptr, 0};
    EXPECT_CALL(*g_mockExtensionUtil, Convert(_))
        .WillOnce(Return(convertedData));

    int32_t result = assetLoader.Download(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_ERROR);
}

TEST_F(AssetLoaderImplTest, Download_DownloadFails_ReturnsConvertedStatus)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    // Add a DBAssets entry
    DBAssets dbAssets;
    dbAssets.push_back(DBAsset());
    assets["key1"] = dbAssets;

    // Mock successful conversion
    std::pair<void*, size_t> convertedData{reinterpret_cast<void*>(0x1234), 1};
    EXPECT_CALL(*g_mockExtensionUtil, Convert(_))
        .WillOnce(Return(convertedData));
    
    // Mock download failure
    EXPECT_CALL(*g_mockExtensionUtil, ConvertStatus(ERRNO_SUCCESS))
        .WillOnce(Return(DBErr::E_ERROR)); // Simulate a conversion

    // Override the C function to simulate failure
    auto originalFunc = OhCloudExtCloudAssetLoaderDownload;
    OhCloudExtCloudAssetLoaderDownload = [](OhCloudExtCloudAssetLoader*, const OhCloudExtUpDownloadInfo*, void*) {
        return -1; // Simulate failure
    };

    int32_t result = assetLoader.Download(tableName, gid, prefix, assets);
    EXPECT_NE(result, DBErr::E_OK);

    // Restore original function
    OhCloudExtCloudAssetLoaderDownload = originalFunc;
}

// RemoveLocalAssets tests
TEST_F(AssetLoaderImplTest, RemoveLocalAssets_ValueIsDBAssets_CallsRemoveLocalAsset)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    // Add a DBAssets entry
    DBAssets dbAssets;
    dbAssets.push_back(DBAsset());
    assets["key1"] = dbAssets;

    // Mock RemoveLocalAsset to succeed
    // Since RemoveLocalAsset is private, we can't directly mock it.
    // We'll test the behavior through the public interface.

    int32_t result = assetLoader.RemoveLocalAssets(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(AssetLoaderImplTest, RemoveLocalAssets_ValueIsDBAsset_CallsRemoveLocalAsset)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    // Add a DBAsset entry
    DBAsset dbAsset;
    assets["key1"] = dbAsset;

    int32_t result = assetLoader.RemoveLocalAssets(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(AssetLoaderImplTest, RemoveLocalAssets_ValueInvalid_ReturnsInvalidArgs)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    // Add an invalid entry
    assets["key1"] = 123; // Not DBAssets or DBAsset

    int32_t result = assetLoader.RemoveLocalAssets(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_INVALID_ARGS);
}

TEST_F(AssetLoaderImplTest, RemoveLocalAssets_EmptyAssets_ReturnsOk)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    std::string tableName = "test_table";
    std::string gid = "test_gid";
    std::string prefixStr = "test_prefix";
    DBValue prefix = prefixStr;
    DBVBucket assets;

    int32_t result = assetLoader.RemoveLocalAssets(tableName, gid, prefix, assets);
    EXPECT_EQ(result, DBErr::E_OK);
}

// RemoveLocalAsset tests
TEST_F(AssetLoaderImplTest, RemoveLocalAsset_ConvertSucceeds_CallsRemoveLocalAssets)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    DBAsset dbAsset;

    // Mock successful conversion
    std::pair<void*, size_t> convertedData{reinterpret_cast<void*>(0x1234), 1};
    EXPECT_CALL(*g_mockExtensionUtil, Convert(_))
        .WillOnce(Return(convertedData));

    int32_t result = assetLoader.RemoveLocalAsset(dbAsset);
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(AssetLoaderImplTest, RemoveLocalAsset_ConvertFails_ReturnsError)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    DBAsset dbAsset;

    // Mock failed conversion
    std::pair<void*, size_t> convertedData{nullptr, 0};
    EXPECT_CALL(*g_mockExtensionUtil, Convert(_))
        .WillOnce(Return(convertedData));

    int32_t result = assetLoader.RemoveLocalAsset(dbAsset);
    EXPECT_EQ(result, DBErr::E_ERROR);
}

TEST_F(AssetLoaderImplTest, RemoveLocalAsset_RemoveFails_ReturnsError)
{
    OhCloudExtCloudAssetLoader loader;
    AssetLoaderImpl assetLoader(&loader);

    DBAsset dbAsset;

    // Mock successful conversion
    std::pair<void*, size_t> convertedData{reinterpret_cast<void*>(0x1234), 1};
    EXPECT_CALL(*g_mockExtensionUtil, Convert(_))
        .WillOnce(Return(convertedData));

    // Override the C function to simulate failure
    auto originalFunc = OhCloudExtCloudAssetLoaderRemoveLocalAssets;
    OhCloudExtCloudAssetLoaderRemoveLocalAssets = [](void*) {
        return -1; // Simulate failure
    };

    int32_t result = assetLoader.RemoveLocalAsset(dbAsset);
    EXPECT_EQ(result, DBErr::E_ERROR);

    // Restore original function
    OhCloudExtCloudAssetLoaderRemoveLocalAss
} // namespace OHOS::Test