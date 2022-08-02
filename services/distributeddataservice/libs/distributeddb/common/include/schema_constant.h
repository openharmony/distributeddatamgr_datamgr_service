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

#ifndef SCHEMA_CONSTANT_H
#define SCHEMA_CONSTANT_H

#include <string>

// This header is supposed to be included only in source files. Do not include it in any header files.
namespace DistributedDB {
class SchemaConstant final {
public:
    static const std::string KEYWORD_SCHEMA_VERSION;
    static const std::string KEYWORD_SCHEMA_MODE;
    static const std::string KEYWORD_SCHEMA_DEFINE;
    static const std::string KEYWORD_SCHEMA_INDEXES;
    static const std::string KEYWORD_SCHEMA_SKIPSIZE;
    static const std::string KEYWORD_SCHEMA_TYPE;
    static const std::string KEYWORD_SCHEMA_TABLE;
    static const std::string KEYWORD_INDEX; // For FlatBuffer-Schema
    static const std::string KEYWORD_TABLE_MODE;

    static const std::string KEYWORD_MODE_STRICT;
    static const std::string KEYWORD_MODE_COMPATIBLE;

    static const std::string KEYWORD_TYPE_BOOL;
    static const std::string KEYWORD_TYPE_INTEGER;
    static const std::string KEYWORD_TYPE_LONG;
    static const std::string KEYWORD_TYPE_DOUBLE;
    static const std::string KEYWORD_TYPE_STRING;

    static const std::string KEYWORD_ATTR_NOT_NULL;
    static const std::string KEYWORD_ATTR_DEFAULT;
    static const std::string KEYWORD_ATTR_VALUE_NULL;
    static const std::string KEYWORD_ATTR_VALUE_TRUE;
    static const std::string KEYWORD_ATTR_VALUE_FALSE;

    static const std::string KEYWORD_TABLE_SPLIT_DEVICE;
    static const std::string KEYWORD_TABLE_COLLABORATION;

    static const std::string KEYWORD_TYPE_RELATIVE;
    static const std::string SCHEMA_SUPPORT_VERSION;
    static const std::string SCHEMA_SUPPORT_VERSION_V2;
    static const std::string SCHEMA_SUPPORT_VERSION_V2_1;
    static const std::string SCHEMA_CURRENT_VERSION;

    static const uint32_t SCHEMA_META_FEILD_COUNT_MAX;
    static const uint32_t SCHEMA_META_FEILD_COUNT_MIN;
    static const uint32_t SCHEMA_FEILD_NAME_LENGTH_MAX;
    static const uint32_t SCHEMA_FEILD_NAME_LENGTH_MIN;
    static const uint32_t SCHEMA_FEILD_NAME_COUNT_MAX;
    static const uint32_t SCHEMA_FEILD_NAME_COUNT_MIN;
    static const uint32_t SCHEMA_FEILD_PATH_DEPTH_MAX;
    static const uint32_t SCHEMA_INDEX_COUNT_MAX;
    static const uint32_t SCHEMA_STRING_SIZE_LIMIT;
    static const uint32_t SCHEMA_DEFAULT_STRING_SIZE_LIMIT;
    static const uint32_t SCHEMA_SKIPSIZE_MAX;

    static const uint32_t SECURE_BYTE_ALIGN;
};
} // namespace DistributedDB
#endif // SCHEMA_CONSTANT_H