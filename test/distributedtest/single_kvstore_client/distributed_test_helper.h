/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_TEST_HELPER_INCLUDE_H
#define DISTRIBUTED_TEST_HELPER_INCLUDE_H

#include <gtest/gtest.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "distributed_kv_data_manager.h"
#include "types.h"
#include "distributed_major.h"
#include "refbase.h"
#include "hilog/log.h"

class DistributedTestHelper : public OHOS::DistributeSystemTest::DistributeTest {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    virtual void SetUp() override;
    virtual void TearDown() override;
    virtual void TestBody() override;

    OHOS::DistributedKv::Status GetRemote(const OHOS::DistributedKv::Key &key,
        OHOS::DistributedKv::Value &value);
    OHOS::DistributedKv::Status PutRemote(const OHOS::DistributedKv::Key &key,
        const OHOS::DistributedKv::Value &value);
    OHOS::DistributedKv::Status DeleteRemote(const OHOS::DistributedKv::Key &key);
    OHOS::DistributedKv::Status RemoveDeviceDataRemote();
    OHOS::DistributedKv::Status SyncRemote(OHOS::DistributedKv::SyncMode mode, uint32_t delay = 0);
};
#endif // DISTRIBUTED_TEST_HELPER_INCLUDE_H