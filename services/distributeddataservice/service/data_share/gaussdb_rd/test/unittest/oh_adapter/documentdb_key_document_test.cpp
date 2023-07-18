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
#include <vector>

#include "doc_errno.h"
#include "document_key.h"
#include "documentdb_test_utils.h"
#include "log_print.h"

using namespace DocumentDB;
using namespace testing::ext;
using namespace DocumentDBUnitTest;

namespace {
class DocumentDBKeyCommonTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBKeyCommonTest::SetUpTestCase(void) {}

void DocumentDBKeyCommonTest::TearDownTestCase(void) {}

void DocumentDBKeyCommonTest::SetUp(void) {}

void DocumentDBKeyCommonTest::TearDown(void) {}

} // namespace