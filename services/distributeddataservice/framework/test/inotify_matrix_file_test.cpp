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

#include "gtest/gtest.h"
#include "inotify_matrix_file/inotify_matrix_file.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class InotifyMatrixFileTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
 * @tc.name: MatrixTableInfo
 * @tc.desc: test MatrixTableInfo Marshal and Unmarshal
 * @tc.type: FUNC
 */
HWTEST_F(InotifyMatrixFileTest, MatrixTableInfo, TestSize.Level1)
{
    MatrixTableInfo tableInfo1;
    tableInfo1.matrixOffset = 1;
    tableInfo1.matrixValue = 2;
    Serializable::json node1;
    tableInfo1.Marshal(node1);
    EXPECT_EQ(node1["matrixOffset"], 1);
    EXPECT_EQ(node1["matrixValue"], 2);

    MatrixTableInfo tableInfo2;
    tableInfo2.Unmarshal(node1);
    EXPECT_EQ(tableInfo2.matrixOffset, 1);
    EXPECT_EQ(tableInfo2.matrixValue, 2);
}

/**
 * @tc.name: MatrixFileInfo
 * @tc.desc: test MatrixFileInfo Marshal and Unmarshal
 * @tc.type: FUNC
 */
HWTEST_F(InotifyMatrixFileTest, MatrixFileInfo, TestSize.Level1)
{
    MatrixFileInfo fileInfo1;
    fileInfo1.version = 1;
    fileInfo1.rdbMetaKey = "metaKey";
    fileInfo1.matrixTables["table1"] = MatrixTableInfo(1, 2);
    Serializable::json node1;
    fileInfo1.Marshal(node1);
    EXPECT_EQ(node1["version"], 1);
    EXPECT_EQ(node1["rdbMetaKey"], "metaKey");

    MatrixFileInfo fileInfo2;
    fileInfo2.Unmarshal(node1);
    EXPECT_EQ(fileInfo2.version, 1);
    EXPECT_EQ(fileInfo2.rdbMetaKey, "metaKey");
    EXPECT_EQ(fileInfo2.matrixTables["table1"].matrixOffset, 1);
    EXPECT_EQ(fileInfo2.matrixTables["table1"].matrixValue, 2);
}

/**
 * @tc.name: GenerateMatrixFileName
 * @tc.desc: test GenerateMatrixFileName function
 * @tc.type: FUNC
 */
HWTEST_F(InotifyMatrixFileTest, GenerateMatrixFileName, TestSize.Level1)
{
    StoreMetaData meta;
    meta.user = "101";
    meta.dataDir = "/data/rdb/aa.db";
    std::string ret1 = MatrixFileInfo::GenerateMatrixFileName(meta);
    EXPECT_EQ(ret1, "matrix_101__data_rdb_aa_db");
}
} // namespace OHOS::Test