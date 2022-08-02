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

#include "schema_constant.h"

namespace DistributedDB {
const std::string SchemaConstant::KEYWORD_SCHEMA_VERSION = "SCHEMA_VERSION";
const std::string SchemaConstant::KEYWORD_SCHEMA_MODE = "SCHEMA_MODE";
const std::string SchemaConstant::KEYWORD_SCHEMA_DEFINE = "SCHEMA_DEFINE";
const std::string SchemaConstant::KEYWORD_SCHEMA_INDEXES = "SCHEMA_INDEXES";
const std::string SchemaConstant::KEYWORD_SCHEMA_SKIPSIZE = "SCHEMA_SKIPSIZE";
const std::string SchemaConstant::KEYWORD_SCHEMA_TYPE = "SCHEMA_TYPE";
const std::string SchemaConstant::KEYWORD_SCHEMA_TABLE = "TABLES";
const std::string SchemaConstant::KEYWORD_INDEX = "INDEX"; // For FlatBuffer-Schema
const std::string SchemaConstant::KEYWORD_TABLE_MODE = "TABLE_MODE";

const std::string SchemaConstant::KEYWORD_MODE_STRICT = "STRICT";
const std::string SchemaConstant::KEYWORD_MODE_COMPATIBLE = "COMPATIBLE";

const std::string SchemaConstant::KEYWORD_TYPE_BOOL = "BOOL";
const std::string SchemaConstant::KEYWORD_TYPE_INTEGER = "INTEGER";
const std::string SchemaConstant::KEYWORD_TYPE_LONG = "LONG";
const std::string SchemaConstant::KEYWORD_TYPE_DOUBLE = "DOUBLE";
const std::string SchemaConstant::KEYWORD_TYPE_STRING = "STRING";

const std::string SchemaConstant::KEYWORD_ATTR_NOT_NULL = "NOT NULL";
const std::string SchemaConstant::KEYWORD_ATTR_DEFAULT = "DEFAULT";
const std::string SchemaConstant::KEYWORD_ATTR_VALUE_NULL = "null";
const std::string SchemaConstant::KEYWORD_ATTR_VALUE_TRUE = "true";
const std::string SchemaConstant::KEYWORD_ATTR_VALUE_FALSE = "false";

const std::string SchemaConstant::KEYWORD_TABLE_SPLIT_DEVICE = "SPLIT_BY_DEVICE";
const std::string SchemaConstant::KEYWORD_TABLE_COLLABORATION = "COLLABORATION";

const std::string SchemaConstant::KEYWORD_TYPE_RELATIVE = "RELATIVE";
const std::string SchemaConstant::SCHEMA_SUPPORT_VERSION = "1.0";
const std::string SchemaConstant::SCHEMA_SUPPORT_VERSION_V2 = "2.0";
const std::string SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1 = "2.1";
const std::string SchemaConstant::SCHEMA_CURRENT_VERSION = SCHEMA_SUPPORT_VERSION_V2_1;

const uint32_t SchemaConstant::SCHEMA_META_FEILD_COUNT_MAX = 5;
const uint32_t SchemaConstant::SCHEMA_META_FEILD_COUNT_MIN = 3;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_LENGTH_MAX = 64;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_LENGTH_MIN = 1;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_COUNT_MAX = 256;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_COUNT_MIN = 1;
const uint32_t SchemaConstant::SCHEMA_FEILD_PATH_DEPTH_MAX = 4;
const uint32_t SchemaConstant::SCHEMA_INDEX_COUNT_MAX = 32;
const uint32_t SchemaConstant::SCHEMA_STRING_SIZE_LIMIT = 524288; // 512K
const uint32_t SchemaConstant::SCHEMA_DEFAULT_STRING_SIZE_LIMIT = 4096; // 4K
const uint32_t SchemaConstant::SCHEMA_SKIPSIZE_MAX = 4194302; // 4M - 2 Bytes

const uint32_t SchemaConstant::SECURE_BYTE_ALIGN = 8; // 8 bytes align
} // namespace DistributedDB
