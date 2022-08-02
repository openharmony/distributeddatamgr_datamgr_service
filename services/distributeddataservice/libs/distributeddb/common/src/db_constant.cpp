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

#include "db_constant.h"

namespace DistributedDB {
const std::string DBConstant::MULTI_SUB_DIR = "multi_ver";
const std::string DBConstant::SINGLE_SUB_DIR = "single_ver";
const std::string DBConstant::LOCAL_SUB_DIR = "local";

const std::string DBConstant::MAINDB_DIR = "main";
const std::string DBConstant::METADB_DIR = "meta";
const std::string DBConstant::CACHEDB_DIR = "cache";

const std::string DBConstant::LOCAL_DATABASE_NAME = "local";
const std::string DBConstant::MULTI_VER_DATA_STORE = "multi_ver_data";
const std::string DBConstant::MULTI_VER_COMMIT_STORE = "commit_logs";
const std::string DBConstant::MULTI_VER_VALUE_STORE = "value_storage";
const std::string DBConstant::MULTI_VER_META_STORE = "meta_storage";
const std::string DBConstant::SINGLE_VER_DATA_STORE = "gen_natural_store";
const std::string DBConstant::SINGLE_VER_META_STORE = "meta";
const std::string DBConstant::SINGLE_VER_CACHE_STORE = "cache";

const std::string DBConstant::SQLITE_URL_PRE = "file:";
const std::string DBConstant::SQLITE_DB_EXTENSION = ".db";
const std::string DBConstant::SQLITE_MEMDB_IDENTIFY = "?mode=memory&cache=shared";

const std::string DBConstant::SCHEMA_KEY = "schemaKey";
const std::string DBConstant::RELATIONAL_SCHEMA_KEY = "relational_schema";

const std::string DBConstant::PATH_POSTFIX_UNPACKED = "_unpacked";
const std::string DBConstant::PATH_POSTFIX_IMPORT_BACKUP = "_import_bak";
const std::string DBConstant::PATH_POSTFIX_IMPORT_ORIGIN = "_import_ori";
const std::string DBConstant::PATH_POSTFIX_IMPORT_DUP = "_import_dup";
const std::string DBConstant::PATH_POSTFIX_EXPORT_BACKUP = "_export_bak";
const std::string DBConstant::PATH_POSTFIX_DB_INCOMPLETE = "_db_incomplete.lock";

const std::string DBConstant::REKEY_FILENAME_POSTFIX_PRE = "_ctrl_pre";
const std::string DBConstant::REKEY_FILENAME_POSTFIX_OK = "_ctrl_ok";
const std::string DBConstant::UPGRADE_POSTFIX = "_upgrade.lock";
const std::string DBConstant::SET_SECOPT_POSTFIX = "_secopt.lock";
const std::string DBConstant::PATH_BACKUP_POSTFIX = "_bak";

const std::string DBConstant::ID_CONNECTOR = "-";

const std::string DBConstant::DELETE_KVSTORE_REMOVING = "_removing";
const std::string DBConstant::DB_LOCK_POSTFIX = ".lock";

const std::string DBConstant::SUBSCRIBE_QUERY_PREFIX = "subscribe_query_";

const std::string DBConstant::TRIGGER_REFERENCES_NEW = "NEW.";
const std::string DBConstant::TRIGGER_REFERENCES_OLD = "OLD.";

const std::string DBConstant::UPDATE_META_FUNC = "update_meta_within_trigger";

const std::string DBConstant::SYSTEM_TABLE_PREFIX = "naturalbase_rdb_";
const std::string DBConstant::RELATIONAL_PREFIX = "naturalbase_rdb_aux_";
const std::string DBConstant::TIMESTAMP_ALIAS = "naturalbase_rdb_aux_timestamp";

const std::string DBConstant::LOG_TABLE_VERSION_1 = "1.0";
const std::string DBConstant::LOG_TABLE_VERSION_2 = "2.0";
const std::string DBConstant::LOG_TABLE_VERSION_CURRENT = "2.0";

const std::string DBConstant::LOG_TABLE_VERSION_KEY = "log_table_version";
}