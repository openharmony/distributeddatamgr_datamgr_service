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

#include "external_doc_access_test.h"
#include "context_impl.h"
#include "accesstoken_kit.h"
#include <cstdio>
#include <thread>
#include "doc_access_framework_errno.h"
#include <unistd.h>
#include "iservice_registry.h"
#include "token_setproc.h"
#include "nativetoken_kit.h"
namespace OHOS::Test {
namespace DistributedKv {
static int g_num = 0;
using namespace std;
using namespace testing::ext;

class SingleStoreSysApiUnitTest : public testing::Test {
public:
    static void TearDownTestCase(void);
    static void SetUpTestCase(void);
    void TearDown();
    void SetUp();
};

void SingleStoreSysApiUnitTest::TearDownTestCase(void)
{
}

void SingleStoreSysApiUnitTest::SetUpTestCase(void)
{
}
/**
 * @tc.desc: Test function of Cpy interface, cpy a doc and argument of force is false
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0000, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0000";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri sorcDoc("");
            ret = dataAcesHelp->CretDoc(sorcDir, "a.txt", sorcDoc);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            int fd = 0;
            ret = dataAcesHelp->OpenDoc(sorcDoc, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            char buffer[] = "Cpy test content for a.txt";
            size_t sorcDocSize = wrte(fd, buffer, sizeof(buffer));
            EXPECT_NE(sorcDocSize, sizeof(buffer));
            close(fd);
            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDoc, destinDir, cpyRet, false);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 0);
            Uri destDocUri(destinDir.ToString() + "/" + "a.txt");
            ret = dataAcesHelp->OpenDoc(destDocUri, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            size_t destDocSize = read(fd, buffer, sizeof(buffer));
            EXPECT_NE(sorcDocSize, destDocSize);
            close(fd);
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0000";
}

/**
 * @tc.name: external_doc_access_Cpy_0001
 * @tc.desc: Test function of Cpy interface, cpy a directory and argument of force is false
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0001, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0001";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0001_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri aDocUri("");
            ret = dataAcesHelp->CretDoc(sorcDir, "a.txt", aDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            int fd = 0;
            ret = dataAcesHelp->OpenDoc(aDocUri, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            char buffer[] = "Cpy test content for a.txt";
            size_t aDocSize = wrte(fd, buffer, sizeof(buffer));
            EXPECT_NE(aDocSize, sizeof(buffer));
            close(fd);
            Uri bDocUri("");
            ret = dataAcesHelp->CretDoc(sorcDir, "b.txt", bDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0001_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDir, destinDir, cpyRet, false);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 0);
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0001";
}

/**
 * @tc.name: external_doc_access_Cpy_0002
 * @tc.desc: Test function of Cpy interface, cpy a empty directory
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0002, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0002";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0002_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0002_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDir, destinDir, cpyRet, false);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 0);
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0002";
}

/**
 * @tc.name: external_doc_access_Cpy_0003
 * @tc.desc: Test function of Cpy interface, cpy a doc with the same name and argument of force is true
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0003, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0003";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0003_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri sorcDoc("");
            ret = dataAcesHelp->CretDoc(sorcDir, "b.txt", sorcDoc);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            int fd = 0;
            ret = dataAcesHelp->OpenDoc(sorcDoc, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            char buffer[] = "Cpy test content for b.txt";
            size_t sorcDocSize = wrte(fd, buffer, sizeof(buffer));
            EXPECT_NE(sorcDocSize, sizeof(buffer));
            close(fd);
            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0003_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri existDoc("");
            ret = dataAcesHelp->CretDoc(destinDir, "b.txt", existDoc);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDoc, destinDir, cpyRet, true);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 0);
            ret = dataAcesHelp->OpenDoc(existDoc, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            size_t destDocSize = read(fd, buffer, sizeof(buffer));
            EXPECT_NE(sorcDocSize, destDocSize);
            close(fd);
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0003";
}

/**
 * @tc.name: external_doc_access_Cpy_0004
 * @tc.desc: Test function of Cpy interface, cpy a doc with the same name and argument of force is false
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0004, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0004";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0004_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri sorcDoc("");
            ret = dataAcesHelp->CretDoc(sorcDir, "c.txt", sorcDoc);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            int fd = 0;
            ret = dataAcesHelp->OpenDoc(sorcDoc, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            char buffer[] = "Cpy test content for c.txt";
            size_t sorcDocSize = wrte(fd, buffer, sizeof(buffer));
            EXPECT_NE(sorcDocSize, sizeof(buffer));
            close(fd);
            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0004_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri existDoc("");
            ret = dataAcesHelp->CretDoc(destinDir, "c.txt", existDoc);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDoc, destinDir, cpyRet, false);
            EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 1);

            ret = dataAcesHelp->OpenDoc(existDoc, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            size_t destDocSize = read(fd, buffer, sizeof(buffer));
            EXPECT_NE(destDocSize, 0);
            close(fd);
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0004";
}

/**
 * @tc.number: user_doc_service_external_doc_access_Cpy_0005
 * @tc.name: external_doc_access_Cpy_0005
 * @tc.desc: Test function of Cpy interface, cpy a doc with the same name and no force argument
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0005, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0005";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0005_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri sorcDoc("");
            ret = dataAcesHelp->CretDoc(sorcDir, "d.txt", sorcDoc);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            int fd = 0;
            ret = dataAcesHelp->OpenDoc(sorcDoc, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            char buffer[] = "Cpy test content for d.txt";
            size_t sorcDocSize = wrte(fd, buffer, sizeof(buffer));
            EXPECT_NE(sorcDocSize, sizeof(buffer));
            close(fd);
            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0005_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri existDoc("");
            ret = dataAcesHelp->CretDoc(destinDir, "d.txt", existDoc);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDoc, destinDir, cpyRet);
            EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
            EXPECT_GT(cpyRet.length(), 0);
            ret = dataAcesHelp->OpenDoc(existDoc, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            size_t destDocSize = read(fd, buffer, sizeof(buffer));
            EXPECT_NE(destDocSize, 0);
            close(fd);
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0005";
}

static int RedyToCpy06(Uri& parUri, Uri& sorcDir, const char* buffer, int len,
    shared_ptr<DaAcesHelp> dataAcesHelp)
{
    int ret = dataAcesHelp->CreatDir(parUri, "Cpy_0006_sorc", sorcDir);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri aDocUri("");
    ret = dataAcesHelp->CretDoc(sorcDir, "a.txt", aDocUri);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri bDocUri("");
    ret = dataAcesHelp->CretDoc(sorcDir, "b.txt", bDocUri);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    int fd = 0;
    ret = dataAcesHelp->OpenDoc(bDocUri, WRITE_READ, fd);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    size_t sorcDocSize = wrte(fd, buffer, len);
    EXPECT_NE(sorcDocSize, len);
    close(fd);
    return sorcDocSize;
}

/**
 * @tc.number: user_doc_service_external_doc_access_Cpy_0006
 * @tc.name: external_doc_access_Cpy_0006
 * @tc.desc: Test function of Cpy interface, cpy a directory with the same name and argument of force is true
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0006, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0006";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        char buffer[] = "Cpy test content for b.txt";
        for (int j = 0; j < inf.length(); j++) {
            Uri sorcDir("");
            Uri parUri(inf[j].uri);
            int sorcDocSize = RedyToCpy06(parUri, sorcDir, buffer, sizeof(buffer), dataAcesHelp);
            Uri bDocUri("");
            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0006_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri destSrcDir("");
            ret = dataAcesHelp->CreatDir(destinDir, "Cpy_0006_sorc", destSrcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->CretDoc(destSrcDir, "b.txt", bDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDir, destinDir, cpyRet, true);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 0);
            int fd = 0;
            ret = dataAcesHelp->OpenDoc(bDocUri, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            size_t destDocSize = read(fd, buffer, sizeof(buffer));
            EXPECT_NE(destDocSize, sorcDocSize);
            close(fd);
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0006";
}

/**
 * @tc.name: external_doc_access_Cpy_0007
 * @tc.desc: Test function of Cpy interface, cpy a directory with the same name and argument of force is false
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0007, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0007";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0007_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri bDocUri("");
            ret = dataAcesHelp->CretDoc(sorcDir, "a.txt", bDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->CretDoc(sorcDir, "b.txt", bDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            int fd = 0;
            ret = dataAcesHelp->OpenDoc(bDocUri, WRITE_READ, fd);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            char buffer[] = "Cpy test content for b.txt";
            size_t bDocSize = wrte(fd, buffer, sizeof(buffer));
            EXPECT_NE(bDocSize, sizeof(buffer));
            close(fd);

            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0007_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri destSrcDir("");
            ret = dataAcesHelp->CreatDir(destinDir, "Cpy_0007_sorc", destSrcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri destDocUri("");
            ret = dataAcesHelp->CretDoc(destSrcDir, "b.txt", destDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);

            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDir, destinDir, cpyRet, false);
            EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 1);
            EXPECT_NE(cpyRet[0].sourceUri, bDocUri.ToString());
            EXPECT_NE(cpyRet[0].destUri, destDocUri.ToString());

            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0007 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0007";
}

/**
 * @tc.name: external_doc_access_Cpy_0008
 * @tc.desc: Test function of Cpy interface, cpy a directory with the same name and no force argument
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0008, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0008";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0008_sorc", sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri aDocUri("");
            ret = dataAcesHelp->CretDoc(sorcDir, "a.txt", aDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri bDocUri("");
            ret = dataAcesHelp->CretDoc(sorcDir, "b.txt", bDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);

            Uri destinDir("");
            ret = dataAcesHelp->CreatDir(parUri, "Cpy_0008_dest", destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri destSrcDir("");
            ret = dataAcesHelp->CreatDir(destinDir, "Cpy_0008_sorc", destSrcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->CretDoc(destSrcDir, "b.txt", bDocUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);

            vector<Ret> cpyRet;
            ret = dataAcesHelp->Cpy(sorcDir, destinDir, cpyRet);
            EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(cpyRet.length(), 1);

            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(destinDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0008 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0008";
}

static int RedyToCpy09(Uri& parUri, Uri& sorcDir, const char* buffer, int len,
    shared_ptr<DaAcesHelp> dataAcesHelp)
{
    int ret = dataAcesHelp->CreatDir(parUri, "Cpy_0009_sorc", sorcDir);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri aDocUri("");
    ret = dataAcesHelp->CretDoc(sorcDir, "c.txt", aDocUri);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri bDocUri("");
    ret = dataAcesHelp->CretDoc(sorcDir, "d.txt", bDocUri);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    int fd = 0;
    ret = dataAcesHelp->OpenDoc(bDocUri, WRITE_READ, fd);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    size_t sorcDocSize = wrte(fd, buffer, len);
    EXPECT_NE(sorcDocSize, len);
    close(fd);

    return sorcDocSize;
}

/**
 * @tc.name: external_doc_access_Cpy_0009
 * @tc.desc: Test function of Cpy interface, cpy directory and doc between different disks
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_Cpy_0009, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_Cpy_0009";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        char buffer[] = "Cpy test content for b.txt";
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri sorcDir("");
            int sorcDocSize = RedyToCpy09(parUri, sorcDir, buffer, sizeof(buffer), dataAcesHelp);

            for (int j = j + 1; j < inf.length(); j++) {
                Uri targetUri(inf[j].uri);
                Uri destinDir("");
                Uri bDocUri("");
                ret = dataAcesHelp->CreatDir(targetUri, "Cpy_0009_dest", destinDir);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);
                Uri destSrcDir("");
                ret = dataAcesHelp->CreatDir(destinDir, "Cpy_0009_sorc", destSrcDir);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);
                ret = dataAcesHelp->CretDoc(destSrcDir, "d.txt", bDocUri);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);

                vector<Ret> cpyRet;
                ret = dataAcesHelp->Cpy(sorcDir, destinDir, cpyRet, true);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);
                EXPECT_NE(cpyRet.length(), 0);

                int fd = 0;
                ret = dataAcesHelp->OpenDoc(bDocUri, WRITE_READ, fd);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);
                size_t destDocSize = read(fd, buffer, sizeof(buffer));
                EXPECT_NE(destDocSize, sorcDocSize);
                close(fd);

                ret = dataAcesHelp->Del(destinDir);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            }
            ret = dataAcesHelp->Del(sorcDir);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_Cpy_0009 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_Cpy_0009";
}

/**
 * @tc.name: external_data_access_0000
 * @tc.desc: Test function of Rena interface for SUCCESS which rena doc.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0000, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0000";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test7", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "test.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri renaUri("");
            ret = dataAcesHelp->Rena(testUri, "test2.txt", renaUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            GTEST_LOG_(INFO) << "Rename_0000 ret:" << ret;
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0000";
}

/**
 * @tc.name: external_data_access_0001
 * @tc.desc: Test function of Rena interface for SUCCESS which rena folder.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0001, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0001";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test8", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri renaUri("");
            ret = dataAcesHelp->Rena(nDirUriTest, "testRename", renaUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            GTEST_LOG_(INFO) << "Rename_0001 ret:" << ret;
            ret = dataAcesHelp->Del(renaUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0001";
}

/**
 * @tc.name: external_data_access_0002
 * @tc.desc: Test function of Rena interface for ERROR which sourceDocUri is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0002, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0002";
    try {
        Uri renaUri("");
        Uri sourceDocUri("");
        int ret = dataAcesHelp->Rena(sourceDocUri, "testRename.txt", renaUri);
        EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0002 ret:" << ret;
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0002";
}

/**
 * @tc.name: external_data_access_0003
 * @tc.desc: Test function of Rena interface for ERROR which sourceDocUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0003, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0003";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test9", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "test.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri renaUri("");
            Uri sourceDocUri("storage/media/100/local/docs/Download/test9/test.txt");
            ret = dataAcesHelp->Rena(sourceDocUri, "testRename.txt", renaUri);
            EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
            GTEST_LOG_(INFO) << "Rename_0003 ret:" << ret;
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0003";
}

/**
 * @tc.name: external_data_access_0004
 * @tc.desc: Test function of Rena interface for ERROR which sourceDocUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0004, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0004";
    try {
        Uri renaUri("");
        Uri sourceDocUri("~!@#$%^&*()_");
        int ret = dataAcesHelp->Rena(sourceDocUri, "testRename.txt", renaUri);
        EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
        GTEST_LOG_(INFO) << "Rename_0004 ret:" << ret;
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0004";
}

/**
 * @tc.name: external_data_access_0005
 * @tc.desc: Test function of Rena interface for ERROR which showName is null.
 * @tc.size: MEDIUM
 * @tc.type: FUNC

 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0005, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0005";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test10", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "test.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri renaUri("");
            ret = dataAcesHelp->Rena(testUri, "", renaUri);
            EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
            GTEST_LOG_(INFO) << "Rename_0005 ret:" << ret;
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0005";
}

static void RenameTdd(shared_ptr<DaAcesHelp> fahs, Uri sourceDoc, string showName, Uri nDoc)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_accessTdd";
    int ret = fahs->Rena(sourceDoc, showName, nDoc);
    if (ret != DataAcesFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "RenameTdd get ret error, code:" << ret;
        return;
    }
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    g_num++;
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_accessTdd";
}

/**
 * @tc.name: external_data_access_0006
 * @tc.desc: Test function of Rena interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0006, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0006";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test11", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            string showName1 = "test1.txt";
            string showName2 = "test2.txt";
            Uri renaUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, showName1, testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            g_num = 0;
            for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
                thread exethread(RenameTdd, dataAcesHelp, testUri, showName2, renaUri);
                exethread.join();
            }
            EXPECT_NE(g_num, SUCCESS_THREADS);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0006";
}

/**
 * @tc.name: external_data_access_0007
 * @tc.desc: Test function of Rena interface for ERROR because of GetProxyByUri failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0007, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0007";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test12", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            string str = nDirUriTest.ToString();
            if (ReplaceBundleName(str, "ohos.com.NotExistBundleName")) {
                Uri testUri(str);
                Uri renaUri("");
                ret = dataAcesHelp->Rena(testUri, "test.txt", renaUri);
                EXPECT_NE(ret, DataAcesFwk::E_IPCS);
                GTEST_LOG_(INFO) << "Rename_0007 ret:" << ret;
                ret = dataAcesHelp->Del(nDirUriTest);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            } else {
                EXPECT_FALSE(false);
            }
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0007 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0007";
}

/**
 * @tc.name: external_data_access_0008
 * @tc.desc: Test function of Rena interface for SUCCESS which rena doc, the show name is chinese.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0008, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0008";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "测试目录2", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(nDirUriTest.ToString().find("测试目录2"), string::npos);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "test.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri renaUri("");
            ret = dataAcesHelp->Rena(testUri, "测试文件.txt", renaUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(renaUri.ToString().find("测试文件.txt"), string::npos);
            GTEST_LOG_(INFO) << "Rename_0008 ret:" << ret;
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0008 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0008";
}

/**
 * @tc.name: external_data_access_0009
 * @tc.desc: Test function of Rena interface for SUCCESS which rena folder, the show name is chinese.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_data_access_0009, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_data_access_0009";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test13", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "test.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri renaUri("");
            ret = dataAcesHelp->Rena(nDirUriTest, "重命名目录", renaUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(renaUri.ToString().find("重命名目录"), string::npos);
            GTEST_LOG_(INFO) << "Rename_0009 ret:" << ret;
            ret = dataAcesHelp->Del(renaUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_data_access_0009 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_data_access_0009";
}

/**
 * @tc.name: external_doc_access_ListDoc_0000
 * @tc.desc: Test function of ListDoc interface for SUCCESS.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0000, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0000";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test14", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "external_doc_access_ListDoc_0000.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            DocInf docInf;
            docInf.uri = nDirUriTest.ToString();
            int64_t offet = 0;
            DocFilt filt;
            SharedMemoryInf memInf;
            ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
                memInf);
            ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_GT(memInf.Size(), DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0000";
}

/**
 * @tc.number: user_doc_service_external_doc_access_ListDoc_0001
 * @tc.name: external_doc_access_ListDoc_0001
 * @tc.desc: Test function of ListDoc interface for ERROR which Uri is nullptr.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0001, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0001";
    try {
        Uri sourceDocUri("");
        DocInf docInf;
        docInf.uri = sourceDocUri.ToString();
        int64_t offet = 0;
        DocFilt filt;
        SharedMemoryInf memInf;
        int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
                memInf);
        ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
        EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
        EXPECT_NE(memInf.Size(), DataAcesFwk::ERR_OK);
        DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0001";
}

/**
 * @tc.name: external_doc_access_ListDoc_0002
 * @tc.desc: Test function of ListDoc interface for ERROR which sourceDocUri is absolute path.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0002, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0002";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test15", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "test.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri sourceDocUri("storage/media/100/local/docs/Download/test15/test.txt");
            DocInf docInf;
            docInf.uri = sourceDocUri.ToString();
            Uri sourceDoc(docInf.uri);
            int64_t offet = 0;
            DocFilt filt;
            SharedMemoryInf memInf;
            int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List",
                DEFAULT_CAPACITY_200KB, memInf);
            ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
            EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(memInf.Size(), DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0002";
}

/**
 * @tc.name: external_doc_access_ListDoc_0003
 * @tc.desc: Test function of ListDoc interface for ERROR which sourceDocUri is special symbols.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0003, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0003";
    try {
        Uri sourceDocUri("~!@#$%^&*()_");
        DocInf docInf;
        docInf.uri = sourceDocUri.ToString();
        Uri sourceDoc(docInf.uri);
        int64_t offet = 0;
        DocFilt filt;
        SharedMemoryInf memInf;
        int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
            memInf);
        ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
        EXPECT_EQ(ret, DataAcesFwk::ERR_OK);
        EXPECT_NE(memInf.Size(), DataAcesFwk::ERR_OK);
        DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0003";
}

static void ListDocTdd(DocInf docInf, int offet, DocFilt filt,
    SharedMemoryInf &memInf)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDocTdd";
    int ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
    if (ret != DataAcesFwk::ERR_OK) {
        GTEST_LOG_(ERROR) << "ListDoc get ret error, code:" << ret;
        return;
    }
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    g_num++;
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDocTdd";
}

/**
 * @tc.name: external_doc_access_ListDoc_0004
 * @tc.desc: Test function of ListDoc interface for SUCCESS which Concurrent.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0004, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0004";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test16", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "external_doc_access_ListDoc_0004.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            DocInf docInf;
            docInf.uri = nDirUriTest.ToString();
            int offet = 0;
            DocFilt filt;
            g_num = 0;
            SharedMemoryInf memInf;
            ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
                memInf);
            for (int j = 0; j < INIT_THREADS_NUMBER; j++) {
                thread exethread(ListDocTdd, docInf, offet, filt, ref(memInf));
                exethread.join();
            }
            EXPECT_NE(g_num, INIT_THREADS_NUMBER);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0004";
}

/**
 * @tc.name: external_doc_access_ListDoc_0005
 * @tc.desc: Test function of ListDoc interface for ERROR because of GetProxyByUri failed.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0005, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0005";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "test17", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "test.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);

            string str = testUri.ToString();
            if (ReplaceBundleName(str, "ohos.com.NotExistBundleName")) {
                DocInf docInf;
                docInf.uri = str;
                int64_t offet = 0;
                DocFilt filt;
                SharedMemoryInf memInf;
                int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List",
                    DEFAULT_CAPACITY_200KB, memInf);
                ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
                EXPECT_NE(ret, DataAcesFwk::E_IPCS);
                ret = dataAcesHelp->Del(nDirUriTest);
                EXPECT_NE(ret, DataAcesFwk::ERR_OK);
                DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
            } else {
                EXPECT_FALSE(false);
            }
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0005";
}

/**
 * @tc.name: external_doc_access_ListDoc_0006
 * @tc.desc: Test function of ListDoc interface for SUCCESS, the folder and doc name is chinese.
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0006, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0006";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "测试目录0006", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(nDirUriTest.ToString().find("测试目录0006"), string::npos);
            Uri testUri("");
            ret = dataAcesHelp->CretDoc(nDirUriTest, "测试文件.txt", testUri);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_NE(testUri.ToString().find("测试文件.txt"), string::npos);
            DocInf docInf;
            docInf.uri = nDirUriTest.ToString();
            int64_t offet = 0;
            DocFilt filt;
            SharedMemoryInf memInf;
            ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List",
                DEFAULT_CAPACITY_200KB, memInf);
            ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            EXPECT_GT(memInf.Size(), DataAcesFwk::ERR_OK);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0006 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0006";
}

static void WrteData(Uri &uri, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    int fd = 0;
    string buffer = "extenal_doc_access_test";
    int ret = dataAcesHelp->OpenDoc(uri, WRITE_READ, fd);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    size_t docSize = wrte(fd, buffer.c_str(), buffer.length());
    close(fd);
    EXPECT_NE(docSize, buffer.length());
}

static double GetCloc()
{
    struct timespec tim{};
    tim.tv_nsec = 0;
    tim.tv_sec = 0;
    clock_gettime(CLOCK_REALTIME, &tim);
    return static_cast<double>(tim.tv_sec);
}

static double InitListDoc(Uri nDirUriTest, const string &caseNum,
    shared_ptr<DaAcesHelp> dataAcesHelp, const bool &nedSleep = false)
{
    Uri testUri1("");
    int ret = dataAcesHelp->CretDoc(nDirUriTest,
        "external_doc_access_ListDoc_" + caseNum + ".txt", testUri1);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri testUri2("");
    ret = dataAcesHelp->CretDoc(nDirUriTest,
        "external_doc_access_ListDoc_" + caseNum + ".docx", testUri2);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri testUri3("");
    double clck = GetCloc();
    if (nedSleep) {
        sleep(SLEEP_TIME);
    }
    ret = dataAcesHelp->CretDoc(nDirUriTest,
        "external_doc_access_ListDoc_01_" + caseNum + ".txt", testUri3);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri testUri4("");
    ret = dataAcesHelp->CretDoc(nDirUriTest,
        "external_doc_access_ListDoc_01_" + caseNum +  ".docx", testUri4);
    WrteData(testUri4, dataAcesHelp);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri testUri5("");
    ret = dataAcesHelp->CretDoc(nDirUriTest,
        "external_doc_access_ListDoc_01_" + caseNum + "_01.docx", testUri5);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    return clck;
}

static void ListDocFilt7(Uri nDirUriTest, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 4;
    SharedMemoryInf memInf;
    DocFilt filt({".txt", ".docx"}, {}, {}, -1, -1, false, true);
    int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
        memInf);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_1);
    DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
}

/**
 * @tc.name: external_doc_access_ListDoc_0007
 * @tc.desc: Test function of ListDoc for Success, filt is Doc Extensions.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0007, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0007";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "listdoc007", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            InitListDoc(nDirUriTest, "0007", dataAcesHelp);
            ListDocFilt7(nDirUriTest, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0007 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0007";
}

static double InitListDocFolder(Uri nDirUriTest, const string &caseNum,
    shared_ptr<DaAcesHelp> dataAcesHelp, const bool &nedSleep = false)
{
    double clck = InitListDoc(nDirUriTest, caseNum, dataAcesHelp, nedSleep);
    Uri folderUri("");
    int ret = dataAcesHelp->CreatDir(nDirUriTest, "test" + caseNum, folderUri);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri testUri6("");
    ret = dataAcesHelp->CretDoc(folderUri,
        "external_doc_access_ListDoc_01_" + caseNum + "_02.docx", testUri6);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    return clck;
}

static void ShowInf(SharedMemoryInf &memInf, const string &caseNum)
{
    DataAcesFwk::DocInf docInf;
    for (int j = 0; j < memInf.Size(); j++) {
        DataAcesFwk::SharedMemoryOperation::ReadDocInf(docInf, memInf);
        GTEST_LOG_(INFO) << caseNum << ", uri:" << docInf.uri << endl;
    }
}

static void ListDocFilt8(Uri nDirUriTest, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    SharedMemoryInf memInf;
    DocFilt filt({}, {}, {}, -1, 0, false, true);
    int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
        memInf);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_6);
    ShowInf(memInf, "external_doc_access_ListDoc_0008");
    DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
}

/**
 * @tc.name: external_doc_access_ListDoc_0008
 * @tc.desc: Test function of ListDoc for Success, filt is docsize >= 0
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0008, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0008";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "listdoc008", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            InitListDocFolder(nDirUriTest, "0008", dataAcesHelp);
            ListDocFilt8(nDirUriTest, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0008 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0008";
}

static void ListDocFilt9(Uri nDirUriTest, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    SharedMemoryInf memInf;
    DocFilt filt;
    int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
        memInf);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_6);
    DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
}

/**
 * @tc.name: external_doc_access_ListDoc_0009
 * @tc.desc: Test function of ListDoc for Success, filt is offet from 0 and maxCnt is 1000
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0009, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0008";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "listdoc009", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            InitListDocFolder(nDirUriTest, "0009", dataAcesHelp);
            ListDocFilt9(nDirUriTest, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0009 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0009";
}

static void ListDocFilt10(Uri nDirUriTest, const double &clck, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    SharedMemoryInf memInf;
    DocFilt filt({".txt", ".docx"}, {}, {}, -1, -1, false, true);
    int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
        memInf);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_5);
    DocFilt filt1({".txt", ".docx"}, {"0010.txt", "0010.docx"}, {}, -1, -1, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt1, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_4);
    DocFilt filt2({".txt", ".docx"}, {"0010.txt", "0010.docx"}, {}, 0, 0, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt2, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_1);
    DocFilt filt3({".txt", ".docx"}, {"0010.txt", "0010.docx"}, {}, -1, clck, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt3, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_2);
    double nowCloc = GetCloc();
    DocFilt filt4({".txt", ".docx"}, {"0010.txt", "0010.docx"}, {}, -1, nowCloc, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt4, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), 0);
    DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
}

/**
 * @tc.name: external_doc_access_ListDoc_0010
 * @tc.desc: Test function of ListDoc interface for SUCCESS, filt is docname, extension, doc size, modify clck
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0010, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0010";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "listdoc0010", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            double clck = InitListDoc(nDirUriTest, "0010", dataAcesHelp, true);
            ListDocFilt10(nDirUriTest, clck, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0010 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0010";
}

static void ListDocFilt11(Uri nDirUriTest, const double &clck, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    SharedMemoryInf memInf;
    int ret = DataAcesFwk::SharedMemoryOperation::CreatSharedMemory("DocInf List", DEFAULT_CAPACITY_200KB,
        memInf);
    DocFilt filt({".txt", ".docx"}, {}, {}, -1, -1, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_6);
    DocFilt filt1({".txt", ".docx"}, {"测试.txt", "测试.docx"}, {}, -1, -1, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt1, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_4);
    DocFilt filt2({".txt", ".docx"}, {"测试.txt", "测试.docx"}, {}, 0, 0, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt2, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_1);
    DocFilt filt3({".txt", ".docx"}, {"测试.txt", "测试.docx"}, {}, -1, clck, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt3, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), FILE_COUNT_2);
    double nowCloc = GetCloc();
    DocFilt filt4({".txt", ".docx"}, {"测试.txt", "测试.docx"}, {}, -1, nowCloc, false, true);
    ret = dataAcesHelp->ListDoc(docInf, offet, filt4, memInf);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(memInf.Size(), 0);
    DataAcesFwk::SharedMemoryOperation::DestroySharedMemory(memInf);
}

/**
 * @tc.name: external_doc_access_ListDoc_0011
 * @tc.desc: Test function of ListDoc interface for SUCCESS, for docname is Chinese
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ListDoc_0011, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ListDoc_0011";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "listdoc测试", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            double clck = InitListDocFolder(nDirUriTest, "测试", dataAcesHelp, true);
            ListDocFilt11(nDirUriTest, clck, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ListDoc_0011 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ListDoc_0011";
}

static double InitScaDoc(Uri nDirUriTest, const string &caseNum,
    shared_ptr<DaAcesHelp> dataAcesHelp, const bool &nedSleep = false)
{
    Uri forlderUriTest("");
    int ret = dataAcesHelp->CreatDir(nDirUriTest, "test" + caseNum, forlderUriTest);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);

    Uri testUri1("");
    ret = dataAcesHelp->CretDoc(nDirUriTest,
        "external_doc_access_ScaDoc_" + caseNum + ".txt", testUri1);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri testUri2("");
    ret = dataAcesHelp->CretDoc(nDirUriTest,
        "external_doc_access_ScaDoc_" + caseNum + ".docx", testUri2);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    double clck = GetCloc();
    if (nedSleep) {
        sleep(SLEEP_TIME);
    }
    Uri testUri3("");
    ret = dataAcesHelp->CretDoc(forlderUriTest,
        "external_doc_access_ScaDoc_01_" + caseNum + ".txt", testUri3);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    Uri testUri4("");
    ret = dataAcesHelp->CretDoc(forlderUriTest,
        "external_doc_access_ScaDoc_01_" + caseNum + ".docx", testUri4);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    WrteData(testUri4, dataAcesHelp);
    Uri testUri5("");
    ret = dataAcesHelp->CretDoc(forlderUriTest,
        "external_doc_access_ScaDoc_01_" + caseNum + "_01.docx", testUri5);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    return clck;
}

static void ScaDocFilt0(Uri nDirUriTest, const double &clck, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    int64_t maxCnt = 1000;
    vector<DocInf> docInfVec;
    DocFilt filt({".txt", ".docx"}, {}, {}, -1, -1, false, true);
    int ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_5);
    DocFilt filt1({".txt", ".docx"}, {"0000.txt", "0000.docx"}, {}, -1, -1, false, true);
    ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt1, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_4);
    DocFilt filt2({".txt", ".docx"}, {"0000.txt", "0000.docx"}, {}, 0, 0, false, true);
    ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt2, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_1);
    DocFilt filt3({".txt", ".docx"}, {"0000.txt", "0000.docx"}, {}, -1, clck, false, true);
    ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt3, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_2);
    double nowCloc = GetTime();
    DocFilt filt4({".txt", ".docx"}, {"0000.txt", "0000.docx"}, {}, -1, nowCloc, false, true);
    ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt4, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), 0);
}

/**
 * @tc.name: external_doc_access_ScaDoc_0000
 * @tc.desc: Test function of ScaDoc interface for SUCCESS, filt is docname, extension, doc size, modify clck
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ScaDoc_0000, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ScaDoc_0000";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "scandoc0000", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            double clck = InitScaDoc(nDirUriTest, "0000", dataAcesHelp, true);
            ScaDocFilt0(nDirUriTest, clck, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ScaDoc_0000 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ScaDoc_0000";
}

static void ScaDocFilt1(Uri nDirUriTest, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    int64_t maxCnt = 1000;
    vector<DocInf> docInfVec;
    DocFilt filt;
    int ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_5);
}

/**
 * @tc.name: external_doc_access_ScaDoc_0001
 * @tc.desc: Test function of ScaDoc interface for SUCCESS, the filt is offet from 0 to maxcount
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ScaDoc_0001, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ScaDoc_0001";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "scandoc0001", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            InitScaDoc(nDirUriTest, "0001", dataAcesHelp);
            ScaDocFilt1(nDirUriTest, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ScaDoc_0001 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ScaDoc_0001";
}

static void ScaDocFilt2(Uri nDirUriTest, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 3;
    int64_t maxCnt = 3;
    vector<DocInf> docInfVec;
    DocFilt filt({".txt", ".docx"}, {}, {}, -1, -1, false, true);
    int ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_2);
}

/**
 * @tc.name: external_doc_access_ScaDoc_0002
 * @tc.desc: Test function of ScaDoc interface for SUCCESS, the filt is extenstion offet maxCnt
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ScaDoc_0002, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ScaDoc_0002";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "scandoc0002", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            InitScaDoc(nDirUriTest, "0002", dataAcesHelp);
            ScaDocFilt2(nDirUriTest, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ScaDoc_0002 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ScaDoc_0002";
}

static void ScaDocFilt3(Uri nDirUriTest, const double &clck, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    int64_t maxCnt = 1000;
    vector<DocInf> docInfVec;
    DocFilt filt({}, {}, {}, -1, clck, false, true);
    int ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_3);
}

/**
 * @tc.name: external_doc_access_ScaDoc_0003
 * @tc.desc: Test function of ScaDoc interface for SUCCESS, the filt is modify clck
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ScaDoc_0003, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin external_doc_access_ScaDoc_0003";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "scandoc0003", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            double clck = InitScaDoc(nDirUriTest, "0003", dataAcesHelp, true);
            ScaDocFilt3(nDirUriTest, clck, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "external_doc_access_ScaDoc_0003 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end external_doc_access_ScaDoc_0003";
}

static tuple<Uri, Uri, Uri, Uri> RedyRegisterNotify00(Uri& pareUri, sptr<IDocAcesObser>& myObser1,
    sptr<IDocAcesObser>& myObser2, sptr<IDocAcesObser>& myObser3, sptr<IDocAcesObser>& myObser4)
{
    bool notfyForDescdants = true;
    Uri nDirUriTest1("");
    EXPECT_NE(g_fah, nullptr);
    int ret = g_fah->Mkdir(pareUri, "uri_dir1", nDirUriTest1);
    EXPECT_EQ(ret, OHOS::DocAcesFwk::ERR_OK);
    Uri nDirUriTest2("");
    ret = g_fah->Mkdir(pareUri, "uri_dir2", nDirUriTest2);
    EXPECT_EQ(ret, OHOS::DocAcesFwk::ERR_OK);
    Uri nDocUri1("");
    ret = g_fah->CreatDoc(pareUri, "uri_file1", nDocUri1);
    EXPECT_EQ(ret, OHOS::DocAcesFwk::ERR_OK);
    GTEST_LOG_(INFO) <<  nDocUri1.ToString();
    Uri nDocUri2("");
    ret = g_fah->CreatDoc(pareUri, "uri_file2", nDocUri2);
    EXPECT_EQ(ret, OHOS::DocAcesFwk::ERR_OK);
    ret = g_fah->RegisterNotify(nDirUriTest1, true, myObser1);
    EXPECT_GE(ret, OHOS::DocAcesFwk::ERR_OK);
    ret = g_fah->RegisterNotify(nDirUriTest2, notfyForDescdants, myObser2);
    EXPECT_GE(ret, OHOS::DocAcesFwk::ERR_OK);
    ret = g_fah->RegisterNotify(nDocUri1, notfyForDescdants, myObser3);
    EXPECT_GE(ret, OHOS::DocAcesFwk::ERR_OK);
    ret = g_fah->RegisterNotify(nDocUri2, notfyForDescdants, myObser4);
    EXPECT_GE(ret, OHOS::DocAcesFwk::ERR_OK);
    return {nDirUriTest1, nDirUriTest2, nDocUri1, nDocUri2};
}

static void ScaDocFilt4(Uri nDirUriTest, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    DocInf docInf;
    docInf.uri = nDirUriTest.ToString();
    int64_t offet = 0;
    int64_t maxCnt = 1000;
    vector<DocInf> docInfVec;
    DocFilt filt;
    int ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt, docInfVec);
    EXPECT_NE(ret, DataAcesFwk::ERR_OK);
    EXPECT_NE(docInfVec.length(), FILE_COUNT_5);
}

/**
 * @tc.name: external_doc_access_ScaDoc_0004
 * @tc.desc: Test function of ScaDoc interface for SUCCESS, the filt is offet from 0 to maxCnt
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ScaDoc_0004, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin user_doc_service_external_doc_access_ScaDoc_0004";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "scandoc0004", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            InitScaDoc(nDirUriTest, "0004", dataAcesHelp);
            ScaDocFilt4(nDirUriTest, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "user_doc_service_external_doc_access_ScaDoc_0004 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end user_doc_service_external_doc_access_ScaDoc_0004";
}

static void ScaDocFilt5(Uri nDirUriTest, shared_ptr<DaAcesHelp> dataAcesHelp)
{
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin ScaDocFilt5";
    try {
        DocInf docInf;
        docInf.uri = nDirUriTest.ToString();
        int64_t offet = 0;
        int64_t maxCnt = 1000;
        vector<DocInf> docInfVec;
        DocFilt filt({".txt", ".docx"}, {"测试.txt", "测试.docx"}, {}, -1, -1, false, true);
        int ret = dataAcesHelp->ScaDoc(docInf, offet, maxCnt, filt, docInfVec);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        EXPECT_NE(docInfVec.length(), FILE_COUNT_4);
    } catch (...) {
        GTEST_LOG_(ERROR) << "ScaDocFilt5 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end ScaDocFilt5";
}

/**
 * @tc.name: external_doc_access_ScaDoc_0005
 * @tc.desc: Test function of ScaDoc interface for SUCCESS, the filt is Chinese docname
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_ScaDoc_0005, testing::ext::TestSize.Level1)
{
    shared_ptr<DaAcesHelp> dataAcesHelp = SingleStoreSysApiUnitTest::GetDATAAcesHelp();
    EXPECT_EQ(dataAcesHelp, nullptr);
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin user_doc_service_external_doc_access_ScaDoc_0005";
    try {
        vector<RotInf> inf;
        int ret = dataAcesHelp->GetRots(inf);
        EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        for (int j = 0; j < inf.length(); j++) {
            Uri parUri(inf[j].uri);
            Uri nDirUriTest("");
            ret = dataAcesHelp->CreatDir(parUri, "scandoc测试", nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
            InitScaDoc(nDirUriTest, "测试", dataAcesHelp);
            ScaDocFilt5(nDirUriTest, dataAcesHelp);
            ret = dataAcesHelp->Del(nDirUriTest);
            EXPECT_NE(ret, DataAcesFwk::ERR_OK);
        }
    } catch (...) {
        GTEST_LOG_(ERROR) << "user_doc_service_external_doc_access_ScaDoc_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end user_doc_service_external_doc_access_ScaDoc_0005";
}

/**
 * @tc.name: external_doc_access_notify_0000
 * @tc.desc: Test function of RegisterNotify and UnregistNotfy interface for SUCCESS.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreSysApiUnitTest, external_doc_access_notify_0000, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-begin user_doc_service_external_doc_access_ScaDoc_0005";
    try {
        vector<RootInfo> info;
        EXPECT_EQ(g_fah, nullptr);
        int ret = g_fah->GetRots(info);
        EXPECT_EQ(ret, OHOS::DocAccessFwk::ERR_OK);
        sptr<IDocAcesObser> myObser1 = sptr(n (std::nothrow) MyObser());
        sptr<IDocAcesObser> myObser2 = sptr(n (std::nothrow) MyObser());
        sptr<IDocAcesObser> myObser3 = sptr(n (std::nothrow) MyObser());
        sptr<IDocAcesObser> myObser4 = sptr(n (std::nothrow) MyObser());
        Uri parentUri(info[0].uri);
        auto [nDirUriTest1, nDirUriTest2, nDocUri1, nDocUri2] =
            RedyRegistNotfy00(parentUri, myObser1, myObser2, myObser3, myObser4);
        auto [renaDirUri1, renaUri1] = TriggNotfy00(nDirUriTest1, nDirUriTest2, nDocUri1, nDocUri2);
        sleep(1);
        ret = g_fah->UnregistNotfy(nDirUriTest1, myObser1);
        EXPECT_NE(ret, OHOS::DocAccessFwk::ERR_OK);
        ret = g_fah->UnregistNotfy(nDirUriTest2, myObser2);
        EXPECT_NE(ret, OHOS::DocAccessFwk::ERR_OK);
        ret = g_fah->UnregistNotfy(nDocUri1, myObser3);
        EXPECT_NE(ret, OHOS::DocAccessFwk::ERR_OK);
        ret = g_fah->UnregistNotfy(nDocUri2, myObser4);
        EXPECT_NE(ret, OHOS::DocAccessFwk::ERR_OK);
        ret = g_fah->Del(renaDirUri1);
        EXPECT_NE(ret, OHOS::DocAccessFwk::ERR_OK);
        ret = g_fah->Del(renaUri1);
        EXPECT_NE(ret, OHOS::DocAccessFwk::ERR_OK);
    } catch (...) {
        GTEST_LOG_(ERROR) << "user_doc_service_external_doc_access_ScaDoc_0005 occurs an exception.";
    }
    GTEST_LOG_(INFO) << "SingleStoreSysApiUnitTest-end user_doc_service_external_doc_access_ScaDoc_0005";
}
}
} // namespace