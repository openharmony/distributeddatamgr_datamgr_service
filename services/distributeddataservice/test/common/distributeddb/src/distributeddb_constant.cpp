/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "distributeddb_constant.h"
#include "distributeddb_nb_test_tools.h"

namespace DistributedDBDataGenerator {
#ifndef HW_USING_LABEL_FUNC_IN_RELEASE_VERSION
    const std::string DistributedDBConstant::NB_DIRECTOR = "/data/test/nbstub/"; // default work dir.
#else
    // default work dir by label in phone requested by fbe.
    const std::string DistributedDBConstant::NB_DIRECTOR = "/data/misc_ce/0/test/";
#endif
    const std::string DistributedDBConstant::NB_DATABASE_NAME = "single_ver/main/gen_natural_store.db";
    const std::string DistributedDBConstant::NORMAL_COMMON_SCHEMA = "normal_common_schema.bfbs";
    const std::string DistributedDBConstant::COMPATIBLE_FOR_NORMAL_COMMON_SCHEMA =
        "compatible_for_normal_common_schema.bfbs";
    const std::string DistributedDBConstant::UNCOMPATIBLE_FOR_NORMAL_COMMON_SCHEMA =
        "uncompatible_for_normal_common_schema.bfbs";
    const DistributedDB::KvStoreConfig DistributedDBConstant::CONFIG = {
        .dataDir = NB_DIRECTOR
    };
    const std::string DistributedDBConstant::DB_FILE_DOCUMENT = "single_ver/";
} // namespace DistributedDBDataGenerator
