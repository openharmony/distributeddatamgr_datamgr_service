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

#ifndef I_SCHEMA_H
#define I_SCHEMA_H

#include <string>

#include "db_types.h"

namespace DistributedDB {
// SchemaType::NONE represent for KV database which do not have schema. Only invalid SchemaObject is NONE type.
// Enum value must not be changed except SchemaType::UNRECOGNIZED.
enum class SchemaType : uint8_t {
    NONE = 0,
    JSON = 1,
    FLATBUFFER = 2,
    RELATIVE = 3,
    UNRECOGNIZED = 4
};

inline SchemaType ReadSchemaType(uint8_t inType)
{
    if (inType >= static_cast<uint8_t>(SchemaType::UNRECOGNIZED)) {
        return SchemaType::UNRECOGNIZED;
    }
    return static_cast<SchemaType>(inType);
}

struct SchemaAttribute {
    FieldType type = FieldType::LEAF_FIELD_NULL;
    bool isIndexable = false;
    bool hasNotNullConstraint = false;
    bool hasDefaultValue = false;
    FieldValue defaultValue; // Has default value in union part and default construction in string part
    std::string customFieldType {}; // Custom field type like BIGINT, DECIMAL, CHARACTER ...
};

class ISchema {
public:
    ISchema() = default;
    virtual ~ISchema() = default;
    virtual int ParseFromSchemaString(const std::string &inSchemaString) = 0;
    virtual bool IsSchemaValid() const = 0;
    virtual SchemaType GetSchemaType() const = 0;
    virtual std::string ToSchemaString() const = 0;
};
}
#endif // I_SCHEMA_H