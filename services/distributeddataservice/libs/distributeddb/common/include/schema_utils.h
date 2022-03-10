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

#ifndef SCHEMA_UTILS_H
#define SCHEMA_UTILS_H

#include "db_types.h"
#include "schema_object.h"

// This header is supposed to be included only in source files. Do not include it in any header files.
namespace DistributedDB {
class SchemaUtils {
public:
    // Check if any invalid exist, parse it into SchemaAttribute if totally valid and return E_OK
    // Number don't support format of scientific notation. SchemaAttribute.isIndexable always set true.
    // Prefix and postfix spaces or tabs is allowed.
    // Customer FieldType will be accepted if useAffinity is true.
    static int ParseAndCheckSchemaAttribute(const std::string &inAttrString, SchemaAttribute &outAttr,
        bool useAffinity = false);

    // Check if any invalid exist, parse it into FieldPath if totally valid and return E_OK
    // Each fieldName of the fieldPath will be check valid as well. Path depth will be check.
    // Prefix and postfix spaces or tabs is allowed. Prefix $. can be not exist.
    // Parameter permitPrefix means whether $. prefix is permited. If not, return E_SCHEMA_PARSE_FAIL.
    static int ParseAndCheckFieldPath(const std::string &inPathString, FieldPath &outPath, bool permitPrefix = true);

    // Return E_OK if it is totally valid. Prefix and postfix spaces or tabs is not allowed.
    static int CheckFieldName(const FieldName &inName);

    // Remove prefix and postfix spaces or tabs
    static std::string Strip(const std::string &inString);

    // Strip the namespace from the full-name, this method mainly for flatbuffer-type schema
    static std::string StripNameSpace(const std::string &inFullName);

    static std::string FieldTypeString(FieldType inType);
    static std::string SchemaTypeString(SchemaType inType);

    // Restore to string representation of fieldPath with $. prefix
    static std::string FieldPathString(const FieldPath &inPath);

    SchemaUtils() = delete;
    ~SchemaUtils() = delete;

private:

    static int SplitSchemaAttribute(const std::string &inAttrString, std::vector<std::string> &outAttrString);
    static int MakeTrans(const std::string &oriContent, size_t &pos);

    static int ParseSchemaAttribute(std::vector<std::string> &attrContext, SchemaAttribute &outAttr, bool useAffinity);

    static int TransformDefaultValue(std::string &defaultContent, SchemaAttribute &outAttr);

    static int TransToDouble(const std::string &defaultContent, SchemaAttribute &outAttr);

    static int TransToInteger(const std::string &defaultContent, SchemaAttribute &outAttr);

    static int TransToLong(const std::string &defaultContent, SchemaAttribute &outAttr);

    static int TransToString(const std::string &defaultContent, SchemaAttribute &outAttr);

    static int TransToBool(const std::string &defaultContent, SchemaAttribute &outAttr);
};
} // namespace DistributedDB
#endif // SCHEMA_UTILS_H