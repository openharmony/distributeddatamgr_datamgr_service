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

#ifndef DISTRIBUTEDDB_TOOLS_TEST_H
#define DISTRIBUTEDDB_TOOLS_TEST_H

#include <mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "db_types.h"
#include "kv_store_changed_data.h"
#include "kv_store_delegate_impl.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_observer.h"
#include "log_print.h"
#include "message.h"
#include "query.h"

namespace DistributedDBTest {
class DistributedDBToolsTest final {
public:
    DistributedDBToolsTest() {}
    ~DistributedDBToolsTest() {}

    static void TestDirInit(std::string &);
    // remove the test db files in the test directory of dir.
    static int RemoveTestDbFiles(const std::string &);
    static int GetCurrentDir(std::string& dir);
};
} // namespace DistributedDBTest
#endif // DISTRIBUTEDDB_TOOLS_TEST_H
