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
}