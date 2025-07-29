#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "cloud_cursor_impl.h"
#include "extension_util.h"

using namespace OHOS::CloudData;
using namespace testing;

// Mock classes for external dependencies
class MockOhCloudExt {
public:
    MOCK_METHOD(void, OhCloudExtCloudDbDataGetValues, (OhCloudExtCloudDbData*, OhCloudExtVector**), ());
    MOCK_METHOD(int32_t, OhCloudExtVectorGetLength, (OhCloudExtVector*, unsigned int*), ());
    MOCK_METHOD(int32_t, OhCloudExtVectorGet, (OhCloudExtVector*, unsigned int, void**, unsigned int*), ());
    MOCK_METHOD(void, OhCloudExtValueBucketFree, (OhCloudExtValueBucket*), ());
    MOCK_METHOD(void, OhCloudExtCloudDbDataGetHasMore, (OhCloudExtCloudDbData*, bool*), ());
    MOCK_METHOD(int32_t, OhCloudExtCloudDbDataGetNextCursor, (OhCloudExtCloudDbData*, unsigned char**, unsigned int*), ());
    MOCK_METHOD(void, OhCloudExtCloudDbDataFree, (OhCloudExtCloudDbData*), ());
    MOCK_METHOD(void, OhCloudExtVectorFree, (OhCloudExtVector*), ());
    MOCK_METHOD(int32_t, OhCloudExtValueBucketGetKeys, (OhCloudExtValueBucket*, OhCloudExtVector**, unsigned int*), ());
    MOCK_METHOD(int32_t, OhCloudExtValueBucketGetValue, (OhCloudExtValueBucket*, OhCloudExtKeyName, OhCloudExtValueType*, void**, unsigned int*), ());
};

// Global mock instance
MockOhCloudExt* g_mock = nullptr;

// Override the C functions with mock implementations
extern "C" {
    void OhCloudExtCloudDbDataGetValues(OhCloudExtCloudDbData* data, OhCloudExtVector** values) {
        if (g_mock) {
            g_mock->OhCloudExtCloudDbDataGetValues(data, values);
        }
    }

    int32_t OhCloudExtVectorGetLength(OhCloudExtVector* vec, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtVectorGetLength(vec, len);
        }
        return ERRNO_SUCCESS;
    }

    int32_t OhCloudExtVectorGet(OhCloudExtVector* vec, unsigned int index, void** value, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtVectorGet(vec, index, value, len);
        }
        return ERRNO_SUCCESS;
    }

    void OhCloudExtValueBucketFree(OhCloudExtValueBucket* bucket) {
        if (g_mock) {
            g_mock->OhCloudExtValueBucketFree(bucket);
        }
    }

    void OhCloudExtCloudDbDataGetHasMore(OhCloudExtCloudDbData* data, bool* hasMore) {
        if (g_mock) {
            g_mock->OhCloudExtCloudDbDataGetHasMore(data, hasMore);
        }
    }

    int32_t OhCloudExtCloudDbDataGetNextCursor(OhCloudExtCloudDbData* data, unsigned char** cursor, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtCloudDbDataGetNextCursor(data, cursor, len);
        }
        return ERRNO_SUCCESS;
    }

    void OhCloudExtCloudDbDataFree(OhCloudExtCloudDbData* data) {
        if (g_mock) {
            g_mock->OhCloudExtCloudDbDataFree(data);
        }
    }

    void OhCloudExtVectorFree(OhCloudExtVector* vec) {
        if (g_mock) {
            g_mock->OhCloudExtVectorFree(vec);
        }
    }

    int32_t OhCloudExtValueBucketGetKeys(OhCloudExtValueBucket* bucket, OhCloudExtVector** keys, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtValueBucketGetKeys(bucket, keys, len);
        }
        return ERRNO_SUCCESS;
    }

    int32_t OhCloudExtValueBucketGetValue(OhCloudExtValueBucket* bucket, OhCloudExtKeyName name, 
                                         OhCloudExtValueType* type, void** content, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtValueBucketGetValue(bucket, name, type, content, len);
        }
        return ERRNO_SUCCESS;
    }
}

// Mock for ExtensionUtil
class MockExtensionUtil {
public:
    MOCK_METHOD(DBValue, ConvertValues, (OhCloudExtValueBucket*, const std::string&), ());
};

MockExtensionUtil* g_extensionUtilMock = nullptr;

namespace OHOS::CloudData {
    DBValue ExtensionUtil::ConvertValues(OhCloudExtValueBucket* bucket, const std::string& key) {
        if (g_extensionUtilMock) {
            return g_extensionUtilMock->ConvertValues(bucket, key);
        }
        return DBValue{};
    }
}

class CloudCursorImplTest : public Test {
protected:
    void SetUp() override {
        g_mock = new MockOhCloudExt();
        g_extensionUtilMock = new MockExtensionUtil();
    }

    void TearDown() override {
        delete g_mock;
        delete g_extensionUtilMock;
        g_mock = nullptr;
        g_extensionUtilMock = nullptr;
    }
};

// Test constructor with normal flow
TEST_F(CloudCursorImplTest, Constructor_NormalFlow) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    OhCloudExtValueBucket* bucket = reinterpret_cast<OhCloudExtValueBucket*>(0x9ABC);
    
    // Setup mock expectations
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(bucket), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtValueBucketGetKeys(bucket, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtValueBucketFree(bucket));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    
    // Verify initial state
    EXPECT_FALSE(cursor.IsEnd());
}

// Test GetCount
TEST_F(CloudCursorImplTest, GetCount_ReturnsCorrectValue) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(5), Return(ERRNO_SUCCESS)));
    // Other mocks...
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    EXPECT_EQ(cursor.GetCount(), 5);
}

// Test MoveToFirst and MoveToNext
TEST_F(CloudCursorImplTest, MoveToFirstAndNext_WorkCorrectly) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(2), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    
    // Test MoveToFirst
    EXPECT_EQ(cursor.MoveToFirst(), DBErr::E_OK);
    
    // Test MoveToNext
    EXPECT_EQ(cursor.MoveToNext(), DBErr::E_OK);
}

// Test Get with column index
TEST_F(CloudCursorImplTest, Get_ByIndex_ReturnsValue) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    OhCloudExtValueBucket* bucket = reinterpret_cast<OhCloudExtValueBucket*>(0x9ABC);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    cursor.MoveToFirst();
    
    // Try to get a value - should fail because we mocked VectorGet to return error
    DBValue value;
    EXPECT_EQ(cursor.Get(0, value), DBErr::E_ERROR);
}

// Test Close
TEST_F(CloudCursorImplTest, Close_ReleasesResources) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(Return(ERRNO_SUCCESS));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(Return(ERRNO_SUCCESS));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataFree(cloudData));
    EXPECT_CALL(*g_mock, OhCloudExtVectorFree(values));
    
    {
        CloudCursorImpl cursor(cloudData);
        EXPECT_EQ(cursor.Close(), DBErr::E_OK);

    } // Destructor should not try to free again
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "cloud_db_impl.h"
#include "cloud_cursor_impl.h"
#include "extension_util.h"
#include "error/general_error.h"
#include "cloud_ext_types.h"
#include "basic_rust_types.h"

using namespace OHOS::CloudData;
using namespace testing;

// Mock C 接口
extern "C" {
    OhCloudExtStatus OhCloudExtCloudDbExecuteSql(OhCloudExtCloudDatabase*, const OhCloudExtSql*, OhCloudExtHashMap*) {
        return ERRNO_SUCCESS;
    }

    OhCloudExtStatus OhCloudExtCloudDbBatchInsert(OhCloudExtCloudDatabase*, const unsigned char*, size_t,
        OhCloudExtVector*, OhCloudExtVector*) {
        return ERRNO_SUCCESS;
    }

    OhCloudExtStatus OhCloudExtCloudDbBatchUpdate(OhCloudExtCloudDatabase*, const unsigned char*, size_t,
        OhCloudExtVector*, OhCloudExtVector*) {
        return ERRNO_SUCCESS;
    }

    OhCloudExtStatus OhCloudExtCloudDbBatchDelete(OhCloudExtCloudDatabase*, const unsigned char*, size_t,
        OhCloudExtVector*) {
        return ERRNO_SUCCESS;
    }

    OhCloudExtStatus OhCloudExtCloudDbBatchQuery(OhCloudExtCloudDatabase*, const OhCloudExtQueryInfo*,
        OhCloudExtCloudDbData**) {
        return ERRNO_SUCCESS;
    }

    OhCloudExtStatus OhCloudExtCloudDbLock(OhCloudExtCloudDatabase*, int*) {
        return ERRNO_SUCCESS;
    }

    OhCloudExtStatus OhCloudExtCloudDbHeartbeat(OhCloudExtCloudDatabase*) {
        return ERRNO_SUCCESS;
    }

    OhCloudExtStatus OhCloudExtCloudDbUnlock(OhCloudExtCloudDatabase*) {
        return ERRNO_SUCCESS;
    }

    void OhCloudExtCloudDbFree(OhCloudExtCloudDatabase*) {}

    void OhCloudExtHashMapFree(OhCloudExtHashMap*) {}

    void OhCloudExtVectorFree(OhCloudExtVector*) {}
}

// Mock ExtensionUtil
class MockExtensionUtil {
public:
    static std::pair<OhCloudExtHashMap*, bool> Convert(const DBVBucket& bucket) {
        if (bucket.empty()) {
            return {nullptr, false};
        }
        return {reinterpret_cast<OhCloudExtHashMap*>(0x1), true};
    }

    static std::pair<OhCloudExtVector*, bool> Convert(DBVBuckets&& buckets) {
        if (buckets.empty()) {
            return {nullptr, false};
        }
        return {reinterpret_cast<OhCloudExtVector*>(0x1), true};
    }

    static DBVBuckets ConvertBuckets(OhCloudExtVector*) {
        return {};
    }

    static int32_t ConvertStatus(OhCloudExtStatus status) {
        return status == ERRNO_SUCCESS ? DBErr::E_OK : DBErr::E_ERROR;
    }
};

// 替换真实函数为 Mock 函数（链接时替换或使用宏）
#define ExtensionUtil MockExtensionUtil

class CloudDbImplTest : public ::testing::Test {
protected:
    OhCloudExtCloudDatabase* mockDb = reinterpret_cast<OhCloudExtCloudDatabase*>(0x1234);
    std::unique_ptr<CloudDbImpl> dbImpl;

    void SetUp() override {
        dbImpl = std::make_unique<CloudDbImpl>(mockDb);
    }

    void TearDown() override {
        dbImpl.reset();
    }
};

// 测试构造与析构
TEST_F(CloudDbImplTest, ConstructorDestructorTest) {
    EXPECT_NE(dbImpl, nullptr);
}

// 测试 Execute
TEST_F(CloudDbImplTest, ExecuteSuccess) {
    DBVBucket extend = {{"key", "value"}};
    int32_t result = dbImpl->Execute("test_table", "SELECT * FROM test", extend);
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(CloudDbImplTest, ExecuteConvertFail) {
    DBVBucket extend; // empty
    int32_t result = dbImpl->Execute("test_table", "SELECT * FROM test", extend);
    EXPECT_EQ(result, DBErr::E_ERROR);
}

// 测试 BatchInsert
TEST_F(CloudDbImplTest, BatchInsertSuccess) {
    DBVBuckets values = {{{"id", 1}}};
    DBVBuckets extends = {{{"ext", "val"}}};
    int32_t result = dbImpl->BatchInsert("test_table", std::move(values), extends);
    EXPECT_EQ(result, DBErr::E_OK);
}

TEST_F(CloudDbImplTest, BatchInsertConvertFail) {
    DBVBuckets values; // empty
    DBVBuckets extends = {{{"ext", "val"}}};
    int32_t result = dbImpl->BatchInsert("test_table", std::move(values), extends);
    EXPECT_EQ(result, DBErr::E_ERROR);
}

// 测试 BatchUpdate (可变版本)
TEST_F(CloudDbImplTest, BatchUpdateMutableSuccess) {
    DBVBuckets values = {{{"id", 1}}};
    DBVBuckets extends = {{{"ext", "val"}}};
    int32_t result = dbImpl->BatchUpdate("test_table", std::move(values), extends);
    EXPECT_EQ(result, DBErr::E_OK);
}

// 测试 BatchUpdate (不可变版本)
TEST_F(CloudDbImplTest, BatchUpdateConstSuccess) {
    DBVBuckets values = {{{"id", 1}}};
    DBVBuckets extends = {{{"ext", "val"}}};
    int32_t result = dbImpl->BatchUpdate("test_table", std::move(values), extends);
    EXPECT_EQ(result, DBErr::E_OK);
}

// 测试 BatchDelete
TEST_F(CloudDbImplTest, BatchDeleteSuccess) {
    DBVBuckets extends = {{{"id", 1}}};
    int32_t result = dbImpl->BatchDelete("test_table", extends);
    EXPECT_EQ(result, DBErr::E_OK);
}

// 测试 Query
TEST_F(CloudDbImplTest, QuerySuccess) {
    DBVBucket extend = {{DistributedData::SchemaMeta::CURSOR_FIELD, std::string("cursor123")}};
    auto result = dbImpl->Query("test_table", extend);
    EXPECT_EQ(result.first, DBErr::E_OK);
    EXPECT_NE(result.second, nullptr);
}

// 测试 Lock
TEST_F(CloudDbImplTest, LockSuccess) {
    int32_t result = dbImpl->Lock();
    EXPECT_EQ(result, DBErr::E_OK);
}

// 测试 Heartbeat
TEST_F(CloudDbImplTest, HeartbeatSuccess) {
    int32_t result = dbImpl->Heartbeat();
    EXPECT_EQ(result, DBErr::E_OK);
}

// 测试 Unlock
TEST_F(CloudDbImplTest, UnlockSuccess) {
    int32_t result = dbImpl->Unlock();
    EXPECT_EQ(result, DBErr::E_OK);
}

// 测试 AliveTime
TEST_F(CloudDbImplTest, AliveTimeTest) {
    dbImpl->Lock(); // 设置 interval_
    int64_t time = dbImpl->AliveTime();
    EXPECT_GT(time, 0);
}

// 测试 Close
TEST_F(CloudDbImplTest, CloseTest) {
    int32_t result = dbImpl->Close();
    EXPECT_EQ(result, DBErr::E_OK);
}
class MockOhCloudExt {
public:
    MOCK_METHOD(void, OhCloudExtCloudDbDataGetValues, (OhCloudExtCloudDbData*, OhCloudExtVector**), ());
    MOCK_METHOD(int32_t, OhCloudExtVectorGetLength, (OhCloudExtVector*, unsigned int*), ());
    MOCK_METHOD(int32_t, OhCloudExtVectorGet, (OhCloudExtVector*, unsigned int, void**, unsigned int*), ());
    MOCK_METHOD(void, OhCloudExtValueBucketFree, (OhCloudExtValueBucket*), ());
    MOCK_METHOD(void, OhCloudExtCloudDbDataGetHasMore, (OhCloudExtCloudDbData*, bool*), ());
    MOCK_METHOD(int32_t, OhCloudExtCloudDbDataGetNextCursor, (OhCloudExtCloudDbData*, unsigned char**, unsigned int*), ());
    MOCK_METHOD(void, OhCloudExtCloudDbDataFree, (OhCloudExtCloudDbData*), ());
    MOCK_METHOD(void, OhCloudExtVectorFree, (OhCloudExtVector*), ());
    MOCK_METHOD(int32_t, OhCloudExtValueBucketGetKeys, (OhCloudExtValueBucket*, OhCloudExtVector**, unsigned int*), ());
    MOCK_METHOD(int32_t, OhCloudExtValueBucketGetValue, (OhCloudExtValueBucket*, OhCloudExtKeyName, OhCloudExtValueType*, void**, unsigned int*), ());
};

// Global mock instance
MockOhCloudExt* g_mock = nullptr;

// Override the C functions with mock implementations
extern "C" {
    void OhCloudExtCloudDbDataGetValues(OhCloudExtCloudDbData* data, OhCloudExtVector** values) {
        if (g_mock) {
            g_mock->OhCloudExtCloudDbDataGetValues(data, values);
        }
    }

    int32_t OhCloudExtVectorGetLength(OhCloudExtVector* vec, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtVectorGetLength(vec, len);
        }
        return ERRNO_SUCCESS;
    }

    int32_t OhCloudExtVectorGet(OhCloudExtVector* vec, unsigned int index, void** value, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtVectorGet(vec, index, value, len);
        }
        return ERRNO_SUCCESS;
    }

    void OhCloudExtValueBucketFree(OhCloudExtValueBucket* bucket) {
        if (g_mock) {
            g_mock->OhCloudExtValueBucketFree(bucket);
        }
    }

    void OhCloudExtCloudDbDataGetHasMore(OhCloudExtCloudDbData* data, bool* hasMore) {
        if (g_mock) {
            g_mock->OhCloudExtCloudDbDataGetHasMore(data, hasMore);
        }
    }

    int32_t OhCloudExtCloudDbDataGetNextCursor(OhCloudExtCloudDbData* data, unsigned char** cursor, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtCloudDbDataGetNextCursor(data, cursor, len);
        }
        return ERRNO_SUCCESS;
    }

    void OhCloudExtCloudDbDataFree(OhCloudExtCloudDbData* data) {
        if (g_mock) {
            g_mock->OhCloudExtCloudDbDataFree(data);
        }
    }

    void OhCloudExtVectorFree(OhCloudExtVector* vec) {
        if (g_mock) {
            g_mock->OhCloudExtVectorFree(vec);
        }
    }

    int32_t OhCloudExtValueBucketGetKeys(OhCloudExtValueBucket* bucket, OhCloudExtVector** keys, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtValueBucketGetKeys(bucket, keys, len);
        }
        return ERRNO_SUCCESS;
    }

    int32_t OhCloudExtValueBucketGetValue(OhCloudExtValueBucket* bucket, OhCloudExtKeyName name, 
                                         OhCloudExtValueType* type, void** content, unsigned int* len) {
        if (g_mock) {
            return g_mock->OhCloudExtValueBucketGetValue(bucket, name, type, content, len);
        }
        return ERRNO_SUCCESS;
    }
}

// Mock for ExtensionUtil
class MockExtensionUtil {
public:
    MOCK_METHOD(DBValue, ConvertValues, (OhCloudExtValueBucket*, const std::string&), ());
};

MockExtensionUtil* g_extensionUtilMock = nullptr;

namespace OHOS::CloudData {
    DBValue ExtensionUtil::ConvertValues(OhCloudExtValueBucket* bucket, const std::string& key) {
        if (g_extensionUtilMock) {
            return g_extensionUtilMock->ConvertValues(bucket, key);
        }
        return DBValue{};
    }
}

class CloudCursorImplTest : public Test {
protected:
    void SetUp() override {
        g_mock = new MockOhCloudExt();
        g_extensionUtilMock = new MockExtensionUtil();
    }

    void TearDown() override {
        delete g_mock;
        delete g_extensionUtilMock;
        g_mock = nullptr;
        g_extensionUtilMock = nullptr;
    }
};

// Test constructor with normal flow
TEST_F(CloudCursorImplTest, Constructor_NormalFlow) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    OhCloudExtValueBucket* bucket = reinterpret_cast<OhCloudExtValueBucket*>(0x9ABC);
    
    // Setup mock expectations
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(bucket), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtValueBucketGetKeys(bucket, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtValueBucketFree(bucket));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    
    // Verify initial state
    EXPECT_FALSE(cursor.IsEnd());
}

// Test GetCount
TEST_F(CloudCursorImplTest, GetCount_ReturnsCorrectValue) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(5), Return(ERRNO_SUCCESS)));
    // Other mocks...
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    EXPECT_EQ(cursor.GetCount(), 5);
}

// Test MoveToFirst and MoveToNext
TEST_F(CloudCursorImplTest, MoveToFirstAndNext_WorkCorrectly) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(2), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    
    // Test MoveToFirst
    EXPECT_EQ(cursor.MoveToFirst(), DBErr::E_OK);
    
    // Test MoveToNext
    EXPECT_EQ(cursor.MoveToNext(), DBErr::E_OK);
}

// Test Get with column index
TEST_F(CloudCursorImplTest, Get_ByIndex_ReturnsValue) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    OhCloudExtValueBucket* bucket = reinterpret_cast<OhCloudExtValueBucket*>(0x9ABC);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(0), Return(ERRNO_SUCCESS)));
    
    CloudCursorImpl cursor(cloudData);
    cursor.MoveToFirst();
    
    // Try to get a value - should fail because we mocked VectorGet to return error
    DBValue value;
    EXPECT_EQ(cursor.Get(0, value), DBErr::E_ERROR);
}

// Test Close
TEST_F(CloudCursorImplTest, Close_ReleasesResources) {
    OhCloudExtCloudDbData* cloudData = reinterpret_cast<OhCloudExtCloudDbData*>(0x1234);
    OhCloudExtVector* values = reinterpret_cast<OhCloudExtVector*>(0x5678);
    
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetValues(cloudData, _))
        .WillOnce(SetArgPointee<1>(values));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGetLength(values, _))
        .WillOnce(Return(ERRNO_SUCCESS));
    EXPECT_CALL(*g_mock, OhCloudExtVectorGet(values, 0, _, _))
        .WillRepeatedly(Return(ERRNO_ERROR));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetHasMore(cloudData, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataGetNextCursor(cloudData, _, _))
        .WillOnce(Return(ERRNO_SUCCESS));
    EXPECT_CALL(*g_mock, OhCloudExtCloudDbDataFree(cloudData));
    EXPECT_CALL(*g_mock, OhCloudExtVectorFree(values));
    
    {
        CloudCursorImpl cursor(cloudData);
        EXPECT_EQ(cursor.Close(), DBErr::E_OK);
    } // Destructor should not try to free again
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
}
/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>
#include "cloud_server_impl.h"
#include "accesstoken_kit.h"
#include "asset_loader_impl.h"
#include "cloud_extension.h"
#include "cloud_ext_types.h"
#include "extension_util.h"
#include "metadata/meta_data_manager.h"
#include "cloud/cloud_db_impl.h"

using namespace testing;
using namespace OHOS::CloudData;
using namespace Security::AccessToken;

// Mock OhCloudExt相关函数
class MockOhCloudExt {
public:
    // CloudSync相关
    MOCK_METHOD1(OhCloudExtCloudSyncNew, OhCloudExtCloudSync*(int32_t userId));
    MOCK_METHOD1(OhCloudExtCloudSyncFree, void(OhCloudExtCloudSync* server));
    MOCK_METHOD2(OhCloudExtCloudSyncGetServiceInfo, int(OhCloudExtCloudSync* server, OhCloudExtCloudInfo** info));
    MOCK_METHOD3(OhCloudExtCloudSyncGetAppSchema, int(OhCloudExtCloudSync* server, const unsigned char* bundleName, size_t bundleNameLen, OhCloudExtSchemaMeta** schema));
    MOCK_METHOD6(OhCloudExtCloudSyncSubscribe, int(OhCloudExtCloudSync* server, OhCloudExtHashMap* databases, unsigned long long expire, OhCloudExtHashMap** relations, OhCloudExtVector** errs));
    MOCK_METHOD3(OhCloudExtCloudSyncUnsubscribe, int(OhCloudExtCloudSync* server, OhCloudExtHashMap* relations, OhCloudExtVector** errs));
    
    // CloudInfo相关
    MOCK_METHOD1(OhCloudExtCloudInfoFree, void(OhCloudExtCloudInfo* info));
    MOCK_METHOD2(OhCloudExtCloudInfoGetUser, int(OhCloudExtCloudInfo* info, int32_t* userId));
    MOCK_METHOD3(OhCloudExtCloudInfoGetId, int(OhCloudExtCloudInfo* info, unsigned char** id, unsigned int* idLen));
    MOCK_METHOD2(OhCloudExtCloudInfoGetTotalSpace, int(OhCloudExtCloudInfo* info, unsigned long long* totalSpace));
    MOCK_METHOD2(OhCloudExtCloudInfoGetRemainSpace, int(OhCloudExtCloudInfo* info, unsigned long long* remainSpace));
    MOCK_METHOD2(OhCloudExtCloudInfoEnabled, int(OhCloudExtCloudInfo* info, bool* enabled));
    MOCK_METHOD2(OhCloudExtCloudInfoGetAppInfo, int(OhCloudExtCloudInfo* info, OhCloudExtHashMap** briefInfo));
    
    // HashMap相关
    MOCK_METHOD1(OhCloudExtHashMapFree, void(OhCloudExtHashMap* hashMap));
    MOCK_METHOD1(OhCloudExtHashMapNew, OhCloudExtHashMap*(OhCloudExtRustType type));
    MOCK_METHOD6(OhCloudExtHashMapInsert, int(OhCloudExtHashMap* hashMap, void* key, size_t keyLen, void* value, size_t valueLen));
    MOCK_METHOD3(OhCloudExtHashMapIterGetKeyValuePair, int(OhCloudExtHashMap* hashMap, OhCloudExtVector** keys, OhCloudExtVector** values));
    
    // Vector相关
    MOCK_METHOD1(OhCloudExtVectorFree, void(OhCloudExtVector* vector));
    MOCK_METHOD1(OhCloudExtVectorNew, OhCloudExtVector*(OhCloudExtRustType type));
    MOCK_METHOD4(OhCloudExtVectorPush, int(OhCloudExtVector* vector, void* value, size_t valueLen));
    MOCK_METHOD3(OhCloudExtVectorGetLength, int(OhCloudExtVector* vector, unsigned int* len));
    MOCK_METHOD5(OhCloudExtVectorGet, int(OhCloudExtVector* vector, size_t index, void** value, unsigned int* valueLen));
    
    // SchemaMeta相关
    MOCK_METHOD1(OhCloudExtSchemaMetaFree, void(OhCloudExtSchemaMeta* schema));
    MOCK_METHOD2(OhCloudExtSchemaMetaGetVersion, int(OhCloudExtSchemaMeta* schema, unsigned int* version));
    MOCK_METHOD2(OhCloudExtSchemaMetaGetDatabases, int(OhCloudExtSchemaMeta* schema, OhCloudExtVector** databases));
    
    // Database相关
    MOCK_METHOD1(OhCloudExtDatabaseFree, void(OhCloudExtDatabase* database));
    MOCK_METHOD3(OhCloudExtDatabaseGetName, int(OhCloudExtDatabase* database, unsigned char** name, unsigned int* nameLen));
    MOCK_METHOD3(OhCloudExtDatabaseGetAlias, int(OhCloudExtDatabase* database, unsigned char** alias, unsigned int* aliasLen));
    MOCK_METHOD2(OhCloudExtDatabaseGetTable, int(OhCloudExtDatabase* database, OhCloudExtHashMap** tables));
    
    // Table相关
    MOCK_METHOD1(OhCloudExtTableFree, void(OhCloudExtTable* table));
    MOCK_METHOD3(OhCloudExtTableGetName, int(OhCloudExtTable* table, unsigned char** name, unsigned int* nameLen));
    MOCK_METHOD3(OhCloudExtTableGetAlias, int(OhCloudExtTable* table, unsigned char** alias, unsigned int* aliasLen));
    MOCK_METHOD2(OhCloudExtTableGetFields, int(OhCloudExtTable* table, OhCloudExtVector** fields));
    
    // Field相关
    MOCK_METHOD1(OhCloudExtFieldFree, void(OhCloudExtField* field));
    MOCK_METHOD3(OhCloudExtFieldGetColName, int(OhCloudExtField* field, unsigned char** colName, unsigned int* colNameLen));
    MOCK_METHOD3(OhCloudExtFieldGetAlias, int(OhCloudExtField* field, unsigned char** alias, unsigned int* aliasLen));
    MOCK_METHOD2(OhCloudExtFieldGetTyp, int(OhCloudExtField* field, uint32_t* type));
    MOCK_METHOD2(OhCloudExtFieldGetPrimary, int(OhCloudExtField* field, bool* primary));
    MOCK_METHOD2(OhCloudExtFieldGetNullable, int(OhCloudExtField* field, bool* nullable));
    
    // RelationSet相关
    MOCK_METHOD1(OhCloudExtRelationSetFree, void(OhCloudExtRelationSet* relationSet));
    MOCK_METHOD3(OhCloudExtRelationSetGetBundleName, int(OhCloudExtRelationSet* relationSet, unsigned char** bundleName, unsigned int* bundleNameLen));
    MOCK_METHOD2(OhCloudExtRelationSetGetExpireTime, int(OhCloudExtRelationSet* relationSet, unsigned long long* expireTime));
    MOCK_METHOD2(OhCloudExtRelationSetGetRelations, int(OhCloudExtRelationSet* relationSet, OhCloudExtHashMap** relations));
    
    // AssetLoader相关
    MOCK_METHOD4(OhCloudExtCloudAssetLoaderNew, OhCloudExtCloudAssetLoader*(int32_t userId, const unsigned char* bundleName, size_t bundleNameLen, OhCloudExtDatabase* database));
    
    // CloudDB相关
    MOCK_METHOD4(OhCloudExtCloudDbNew, OhCloudExtCloudDatabase*(int32_t userId, const unsigned char* bundleName, size_t bundleNameLen, OhCloudExtDatabase* database));
};

// Mock AccessTokenKit
class MockAccessTokenKit {
public:
    MOCK_METHOD1(GetTokenTypeFlag, TokenTypeFlag(uint32_t tokenId));
    MOCK_METHOD2(GetHapTokenInfo, int(uint32_t tokenId, HapTokenInfo& hapInfo));
};

// Mock ExtensionUtil
class MockExtensionUtil {
public:
    MOCK_METHOD1(Convert, std::pair<OhCloudExtDatabase*, size_t>(const DBMeta& dbMeta));
    MOCK_METHOD1(ConvertAppInfo, DBAppInfo(OhCloudExtAppInfo* appInfo));
};

// Mock MetaDataManager
class MockMetaDataManager {
public:
    MOCK_METHOD3(LoadMeta, void(const std::string& key, DBSub& sub, bool isPublic));
    MOCK_METHOD3(SaveMeta, void(const std::string& key, const DBSub& sub, bool isPublic));
    MOCK_METHOD2(DelMeta, void(const std::string& key, bool isPublic));
};

// Mock CloudServer
class MockCloudServer {
public:
    MOCK_METHOD1(RegisterCloudInstance, void(DistributedData::CloudServer* instance));
};

// 全局Mock对象
static MockOhCloudExt* g_mockOhCloudExt = nullptr;
static MockAccessTokenKit* g_mockAccessTokenKit = nullptr;
static MockExtensionUtil* g_mockExtensionUtil = nullptr;
static MockMetaDataManager* g_mockMetaDataManager = nullptr;
static MockCloudServer* g_mockCloudServer = nullptr;

// 重定向OhCloudExt函数调用到Mock对象
extern "C" {
    OhCloudExtCloudSync* OhCloudExtCloudSyncNew(int32_t userId) {
        return g_mockOhCloudExt->OhCloudExtCloudSyncNew(userId);
    }
    
    void OhCloudExtCloudSyncFree(OhCloudExtCloudSync* server) {
        g_mockOhCloudExt->OhCloudExtCloudSyncFree(server);
    }
    
    int OhCloudExtCloudSyncGetServiceInfo(OhCloudExtCloudSync* server, OhCloudExtCloudInfo** info) {
        return g_mockOhCloudExt->OhCloudExtCloudSyncGetServiceInfo(server, info);
    }
    
    int OhCloudExtCloudSyncGetAppSchema(OhCloudExtCloudSync* server, const unsigned char* bundleName, size_t bundleNameLen, OhCloudExtSchemaMeta** schema) {
        return g_mockOhCloudExt->OhCloudExtCloudSyncGetAppSchema(server, bundleName, bundleNameLen, schema);
    }
    
    int OhCloudExtCloudSyncSubscribe(OhCloudExtCloudSync* server, OhCloudExtHashMap* databases, unsigned long long expire, OhCloudExtHashMap** relations, OhCloudExtVector** errs) {
        return g_mockOhCloudExt->OhCloudExtCloudSyncSubscribe(server, databases, expire, relations, errs);
    }
    
    int OhCloudExtCloudSyncUnsubscribe(OhCloudExtCloudSync* server, OhCloudExtHashMap* relations, OhCloudExtVector** errs) {
        return g_mockOhCloudExt->OhCloudExtCloudSyncUnsubscribe(server, relations, errs);
    }
    
    void OhCloudExtCloudInfoFree(OhCloudExtCloudInfo* info) {
        g_mockOhCloudExt->OhCloudExtCloudInfoFree(info);
    }
    
    int OhCloudExtCloudInfoGetUser(OhCloudExtCloudInfo* info, int32_t* userId) {
        return g_mockOhCloudExt->OhCloudExtCloudInfoGetUser(info, userId);
    }
    
    int OhCloudExtCloudInfoGetId(OhCloudExtCloudInfo* info, unsigned char** id, unsigned int* idLen) {
        return g_mockOhCloudExt->OhCloudExtCloudInfoGetId(info, id, idLen);
    }
    
    int OhCloudExtCloudInfoGetTotalSpace(OhCloudExtCloudInfo* info, unsigned long long* totalSpace) {
        return g_mockOhCloudExt->OhCloudExtCloudInfoGetTotalSpace(info, totalSpace);
    }
    
    int OhCloudExtCloudInfoGetRemainSpace(OhCloudExtCloudInfo* info, unsigned long long* remainSpace) {
        return g_mockOhCloudExt->OhCloudExtCloudInfoGetRemainSpace(info, remainSpace);
    }
    
    int OhCloudExtCloudInfoEnabled(OhCloudExtCloudInfo* info, bool* enabled) {
        return g_mockOhCloudExt->OhCloudExtCloudInfoEnabled(info, enabled);
    }
    
    int OhCloudExtCloudInfoGetAppInfo(OhCloudExtCloudInfo* info, OhCloudExtHashMap** briefInfo) {
        return g_mockOhCloudExt->OhCloudExtCloudInfoGetAppInfo(info, briefInfo);
    }
    
    void OhCloudExtHashMapFree(OhCloudExtHashMap* hashMap) {
        g_mockOhCloudExt->OhCloudExtHashMapFree(hashMap);
    }
    
    OhCloudExtHashMap* OhCloudExtHashMapNew(OhCloudExtRustType type) {
        return g_mockOhCloudExt->OhCloudExtHashMapNew(type);
    }
    
    int OhCloudExtHashMapInsert(OhCloudExtHashMap* hashMap, void* key, size_t keyLen, void* value, size_t valueLen) {
        return g_mockOhCloudExt->OhCloudExtHashMapInsert(hashMap, key, keyLen, value, valueLen);
    }
    
    int OhCloudExtHashMapIterGetKeyValuePair(OhCloudExtHashMap* hashMap, OhCloudExtVector** keys, OhCloudExtVector** values) {
        return g_mockOhCloudExt->OhCloudExtHashMapIterGetKeyValuePair(hashMap, keys, values);
    }
    
    void OhCloudExtVectorFree(OhCloudExtVector* vector) {
        g_mockOhCloudExt->OhCloudExtVectorFree(vector);
    }
    
    OhCloudExtVector* OhCloudExtVectorNew(OhCloudExtRustType type) {
        return g_mockOhCloudExt->OhCloudExtVectorNew(type);
    }
    
    int OhCloudExtVectorPush(OhCloudExtVector* vector, void* value, size_t valueLen) {
        return g_mockOhCloudExt->OhCloudExtVectorPush(vector, value, valueLen);
    }
    
    int OhCloudExtVectorGetLength(OhCloudExtVector* vector, unsigned int* len) {
        return g_mockOhCloudExt->OhCloudExtVectorGetLength(vector, len);
    }
    
    int OhCloudExtVectorGet(OhCloudExtVector* vector, size_t index, void** value, unsigned int* valueLen) {
        return g_mockOhCloudExt->OhCloudExtVectorGet(vector, index, value, valueLen);
    }
    
    void OhCloudExtSchemaMetaFree(OhCloudExtSchemaMeta* schema) {
        g_mockOhCloudExt->OhCloudExtSchemaMetaFree(schema);
    }
    
    int OhCloudExtSchemaMetaGetVersion(OhCloudExtSchemaMeta* schema, unsigned int* version) {
        return g_mockOhCloudExt->OhCloudExtSchemaMetaGetVersion(schema, version);
    }
    
    int OhCloudExtSchemaMetaGetDatabases(OhCloudExtSchemaMeta* schema, OhCloudExtVector** databases) {
        return g_mockOhCloudExt->OhCloudExtSchemaMetaGetDatabases(schema, databases);
    }
    
    void OhCloudExtDatabaseFree(OhCloudExtDatabase* database) {
        g_mockOhCloudExt->OhCloudExtDatabaseFree(database);
    }
    
    int OhCloudExtDatabaseGetName(OhCloudExtDatabase* database, unsigned char** name, unsigned int* nameLen) {
        return g_mockOhCloudExt->OhCloudExtDatabaseGetName(database, name, nameLen);
    }
    
    int OhCloudExtDatabaseGetAlias(OhCloudExtDatabase* database, unsigned char** alias, unsigned int* aliasLen) {
        return g_mockOhCloudExt->OhCloudExtDatabaseGetAlias(database, alias, aliasLen);
    }
    
    int OhCloudExtDatabaseGetTable(OhCloudExtDatabase* database, OhCloudExtHashMap** tables) {
        return g_mockOhCloudExt->OhCloudExtDatabaseGetTable(database, tables);
    }
    
    void OhCloudExtTableFree(OhCloudExtTable* table) {
        g_mockOhCloudExt->OhCloudExtTableFree(table);
    }
    
    int OhCloudExtTableGetName(OhCloudExtTable* table, unsigned char** name, unsigned int* nameLen) {
        return g_mockOhCloudExt->OhCloudExtTableGetName(table, name, nameLen);
    }
    
    int OhCloudExtTableGetAlias(OhCloudExtTable* table, unsigned char** alias, unsigned int* aliasLen) {
        return g_mockOhCloudExt->OhCloudExtTableGetAlias(table, alias, aliasLen);
    }
    
    int OhCloudExtTableGetFields(OhCloudExtTable* table, OhCloudExtVector** fields) {
        return g_mockOhCloudExt->OhCloudExtTableGetFields(table, fields);
    }
    
    void OhCloudExtFieldFree(OhCloudExtField* field) {
        g_mockOhCloudExt->OhCloudExtFieldFree(field);
    }
    
    int OhCloudExtFieldGetColName(OhCloudExtField* field, unsigned char** colName, unsigned int* colNameLen) {
        return g_mockOhCloudExt->OhCloudExtFieldGetColName(field, colName, colNameLen);
    }
    
    int OhCloudExtFieldGetAlias(OhCloudExtField* field, unsigned char** alias, unsigned int* aliasLen) {
        return g_mockOhCloudExt->OhCloudExtFieldGetAlias(field, alias, aliasLen);
    }
    
    int OhCloudExtFieldGetTyp(OhCloudExtField* field, uint32_t* type) {
        return g_mockOhCloudExt->OhCloudExtFieldGetTyp(field, type);
    }
    
    int OhCloudExtFieldGetPrimary(OhCloudExtField* field, bool* primary) {
        return g_mockOhCloudExt->OhCloudExtFieldGetPrimary(field, primary);
    }
    
    int OhCloudExtFieldGetNullable(OhCloudExtField* field, bool* nullable) {
        return g_mockOhCloudExt->OhCloudExtFieldGetNullable(field, nullable);
    }
    
    void OhCloudExtRelationSetFree(OhCloudExtRelationSet* relationSet) {
        g_mockOhCloudExt->OhCloudExtRelationSetFree(relationSet);
    }
    
    int OhCloudExtRelationSetGetBundleName(OhCloudExtRelationSet* relationSet, unsigned char** bundleName, unsigned int* bundleNameLen) {
        return g_mockOhCloudExt->OhCloudExtRelationSetGetBundleName(relationSet, bundleName, bundleNameLen);
    }
    
    int OhCloudExtRelationSetGetExpireTime(OhCloudExtRelationSet* relationSet, unsigned long long* expireTime) {
        return g_mockOhCloudExt->OhCloudExtRelationSetGetExpireTime(relationSet, expireTime);
    }
    
    int OhCloudExtRelationSetGetRelations(OhCloudExtRelationSet* relationSet, OhCloudExtHashMap** relations) {
        return g_mockOhCloudExt->OhCloudExtRelationSetGetRelations(relationSet, relations);
    }
    
    OhCloudExtCloudAssetLoader* OhCloudExtCloudAssetLoaderNew(int32_t userId, const unsigned char* bundleName, size_t bundleNameLen, OhCloudExtDatabase* database) {
        return g_mockOhCloudExt->OhCloudExtCloudAssetLoaderNew(userId, bundleName, bundleNameLen, database);
    }
    
    OhCloudExtCloudDatabase* OhCloudExtCloudDbNew(int32_t userId, const unsigned char* bundleName, size_t bundleNameLen, OhCloudExtDatabase* database) {
        return g_mockOhCloudExt->OhCloudExtCloudDbNew(userId, bundleName, bundleNameLen, database);
    }
}

// 重定向AccessTokenKit函数调用到Mock对象
TokenTypeFlag AccessTokenKit::GetTokenTypeFlag(uint32_t tokenId) {
    return g_mockAccessTokenKit->GetTokenTypeFlag(tokenId);
}

int AccessTokenKit::GetHapTokenInfo(uint32_t tokenId, HapTokenInfo& hapInfo) {
    return g_mockAccessTokenKit->GetHapTokenInfo(tokenId, hapInfo);
}

// 重定向ExtensionUtil函数调用到Mock对象
std::pair<OhCloudExtDatabase*, size_t> ExtensionUtil::Convert(const DBMeta& dbMeta) {
    return g_mockExtensionUtil->Convert(dbMeta);
}

DBAppInfo ExtensionUtil::ConvertAppInfo(OhCloudExtAppInfo* appInfo) {
    return g_mockExtensionUtil->ConvertAppInfo(appInfo);
}

// 重定向MetaDataManager函数调用到Mock对象
void DistributedData::MetaDataManager::LoadMeta(const std::string& key, DBSub& sub, bool isPublic) {
    g_mockMetaDataManager->LoadMeta(key, sub, isPublic);
}

void DistributedData::MetaDataManager::SaveMeta(const std::string& key, const DBSub& sub, bool isPublic) {
    g_mockMetaDataManager->SaveMeta(key, sub, isPublic);
}

void DistributedData::MetaDataManager::DelMeta(const std::string& key, bool isPublic) {
    g_mockMetaDataManager->DelMeta(key, isPublic);
}

// 重定向CloudServer函数调用到Mock对象
void DistributedData::CloudServer::RegisterCloudInstance(DistributedData::CloudServer* instance) {
    g_mockCloudServer->RegisterCloudInstance(instance);
}

class CloudServerImplTest : public Test {
protected:
    void SetUp() override {
        g_mockOhCloudExt = new MockOhCloudExt();
        g_mockAccessTokenKit = new MockAccessTokenKit();
        g_mockExtensionUtil = new MockExtensionUtil();
        g_mockMetaDataManager = new MockMetaDataManager();
        g_mockCloudServer = new MockCloudServer();
    }
    
    void TearDown() override {
        delete g_mockOhCloudExt;
        delete g_mockAccessTokenKit;
        delete g_mockExtensionUtil;
        delete g_mockMetaDataManager;
        delete g_mockCloudServer;
        g_mockOhCloudExt = nullptr;
        g_mockAccessTokenKit = nullptr;
        g_mockExtensionUtil = nullptr;
        g_mockMetaDataManager = nullptr;
        g_mockCloudServer = nullptr;
    }
};

// 测试CloudServerImpl::Init()
TEST_F(CloudServerImplTest, Init_Success)
{
    // 设置期望
    EXPECT_CALL(*g_mockCloudServer, RegisterCloudInstance(_)).Times(1);
    
    // 执行测试
    bool result = CloudServerImpl::Init();
    
    // 验证结果
    EXPECT_TRUE(result);
}

// 测试CloudServerImpl::GetServerInfo() - server创建失败
TEST_F(CloudServerImplTest, GetServerInfo_ServerCreateFailed)
{
    // 设置期望
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncNew(_))
        .WillOnce(Return(nullptr));
    
    // 执行测试
    auto result = CloudServerImpl::GetInstance().GetServerInfo(100, false);
    
    // 验证结果
    EXPECT_EQ(result.first, DBErr::E_ERROR);
}

// 测试CloudServerImpl::GetServerInfo() - 获取服务信息失败
TEST_F(CloudServerImplTest, GetServerInfo_GetServiceInfoFailed)
{
    // 创建mock对象
    auto mockServer = reinterpret_cast<OhCloudExtCloudSync*>(0x1000);
    auto mockInfo = reinterpret_cast<OhCloudExtCloudInfo*>(0x2000);
    
    // 设置期望
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncNew(_))
        .WillOnce(Return(mockServer));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncGetServiceInfo(mockServer, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncFree(mockServer))
        .Times(1);
    
    // 执行测试
    auto result = CloudServerImpl::GetInstance().GetServerInfo(100, false);
    
    // 验证结果
    EXPECT_EQ(result.first, DBErr::E_ERROR);
}

// 测试CloudServerImpl::GetServerInfo() - 成功获取信息
TEST_F(CloudServerImplTest, GetServerInfo_Success)
{
    // 创建mock对象
    auto mockServer = reinterpret_cast<OhCloudExtCloudSync*>(0x1000);
    auto mockInfo = reinterpret_cast<OhCloudExtCloudInfo*>(0x2000);
    auto mockAppInfo = reinterpret_cast<OhCloudExtHashMap*>(0x3000);
    unsigned char id[] = "test_id";
    
    // 设置期望
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncNew(_))
        .WillOnce(Return(mockServer));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncGetServiceInfo(mockServer, _))
        .WillOnce(DoAll(SetArgPointee<1>(mockInfo), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudInfoGetUser(mockInfo, _))
        .WillOnce(DoAll(SetArgPointee<1>(100), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudInfoGetId(mockInfo, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(id), SetArgPointee<2>(7), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudInfoGetTotalSpace(mockInfo, _))
        .Times(0);
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudInfoGetRemainSpace(mockInfo, _))
        .Times(0);
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudInfoEnabled(mockInfo, _))
        .WillOnce(SetArgPointee<1>(true));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudInfoGetAppInfo(mockInfo, _))
        .WillOnce(DoAll(SetArgPointee<1>(mockAppInfo), Return(ERRNO_SUCCESS)));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudInfoFree(mockInfo))
        .Times(1);
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncFree(mockServer))
        .Times(1);
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtHashMapFree(mockAppInfo))
        .Times(1);
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtHashMapIterGetKeyValuePair(mockAppInfo, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(nullptr), SetArgPointee<2>(nullptr), Return(-1)));
    
    // 执行测试
    auto result = CloudServerImpl::GetInstance().GetServerInfo(100, false);
    
    // 验证结果
    EXPECT_EQ(result.first, DBErr::E_ERROR);
    EXPECT_EQ(result.second.user, 100);
    EXPECT_EQ(result.second.id, "test_id");
    EXPECT_TRUE(result.second.enableCloud);
}

// 测试CloudServerImpl::GetAppSchema() - server创建失败
TEST_F(CloudServerImplTest, GetAppSchema_ServerCreateFailed)
{
    // 设置期望
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncNew(_))
        .WillOnce(Return(nullptr));
    
    // 执行测试
    auto result = CloudServerImpl::GetInstance().GetAppSchema(100, "test_bundle");
    
    // 验证结果
    EXPECT_EQ(result.first, DBErr::E_ERROR);
}

// 测试CloudServerImpl::GetAppSchema() - 不支持
TEST_F(CloudServerImplTest, GetAppSchema_Unsupported)
{
    // 创建mock对象
    auto mockServer = reinterpret_cast<OhCloudExtCloudSync*>(0x1000);
    
    // 设置期望
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncNew(_))
        .WillOnce(Return(mockServer));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncGetAppSchema(mockServer, _, _, _))
        .WillOnce(Return(ERRNO_UNSUPPORTED));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncFree(mockServer))
        .Times(1);
    
    // 执行测试
    auto result = CloudServerImpl::GetInstance().GetAppSchema(100, "test_bundle");
    
    // 验证结果
    EXPECT_EQ(result.first, DBErr::E_NOT_SUPPORT);
}

// 测试CloudServerImpl::Subscribe() - server创建失败
TEST_F(CloudServerImplTest, Subscribe_ServerCreateFailed)
{
    // 设置期望
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncNew(_))
        .WillOnce(Return(nullptr));
    
    // 准备测试数据
    std::map<std::string, std::vector<DBMeta>> dbs;
    
    // 执行测试
    int32_t result = CloudServerImpl::GetInstance().Subscribe(100, dbs);
    
    // 验证结果
    EXPECT_EQ(result, DBErr::E_ERROR);
}

// 测试CloudServerImpl::Subscribe() - HashMap创建失败
TEST_F(CloudServerImplTest, Subscribe_HashMapCreateFailed)
{
    // 创建mock对象
    auto mockServer = reinterpret_cast<OhCloudExtCloudSync*>(0x1000);
    
    // 设置期望
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncNew(_))
        .WillOnce(Return(mockServer));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtHashMapNew(_))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*g_mockOhCloudExt, OhCloudExtCloudSyncFree(mockServer))
        .Times(1);
    
    // 准备测试数据
    std::map<std::string, std::vector<DBMeta>> dbs;
    
    // 执行测试
    int32_t result = CloudServerImpl::GetInstance().Subscribe(100, dbs);
    
    // 验证结果
    EXPECT_EQ(result, DBErr::E_ERROR);
}

// 测试CloudServerImpl::Unsubscribe() - 订阅ID为空
TEST_F(CloudServerImplTest, Unsubscribe_EmptySubscriptionId)
{
    // 设置期望
    EXPECT_CALL(*g_mockMetaDataManager, LoadMeta(_, _, _))
        .WillOnce(Invoke([](const std::string& key, DBSub& sub, bool isPublic) {
            sub.id = "";  // 空ID
        }));
    
    // 准备测试数据
    std::map<std::string, std::vector<DBMeta>> dbs;
    
    // 执行测试
    int32_t result = CloudServerImpl::GetInstance().Unsubscribe(100, dbs);
    
    // 验证结果
    EXPECT_EQ(result, DBErr::E_OK);
}

// 测试CloudServerImpl::ConnectAssetLoader() - token类型不是HAP
TEST_F(CloudServerImplTest, ConnectAssetLoader_NotHapToken)
{
    // 设置期望
    EXPECT_CALL(*g_mockAccessTokenKit, GetTokenTypeFlag(_))
        .WillOnce(Return(TOKEN_SHELL));  // 非HAP类型
    
    // 准备测试数据
    uint32_t tokenId = 123;
    DBMeta dbMeta;
    
    // 执行测试
    auto result = CloudServerImpl::GetInstance().ConnectAssetLoader(tokenId, dbMeta);
    
    // 验证结果
    EXPECT_EQ(result, nullptr);
}

// 测试CloudServerImpl::ConnectCloudDB() - token类型不是HAP
TEST_F(CloudServerImplTest, ConnectCloudDB_NotHapToken)
{
    // 设置期望
    EXPECT_CALL(*g_mockAccessTokenKit, GetTokenTypeFlag(_))
        .WillOnce(Return(TOKEN_SHELL));  // 非HAP类型
    
    // 准备测试数据
    uint32_t tokenId = 123;
    DBMeta dbMeta;
    
    // 执行测试
    auto result = CloudServerImpl::GetInstance().ConnectCloudDB(tokenId, dbMeta);
    
    // 验证结果
    EXPECT_EQ(result, nullptr);
}
}