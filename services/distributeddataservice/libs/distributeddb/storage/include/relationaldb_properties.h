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
#ifndef RELATIONALDB_PROPERTIES_H
#define RELATIONALDB_PROPERTIES_H
#ifdef RELATIONAL_STORE
#include <map>
#include <string>
#include <vector>

#include "db_properties.h"
#include "relational_schema_object.h"

namespace DistributedDB {
class RelationalDBProperties final : public DBProperties {
public:
    RelationalDBProperties();
    ~RelationalDBProperties() override;

    // is schema exist
    bool IsSchemaExist() const;

    // set schema
    void SetSchema(const RelationalSchemaObject &schema);

    // get schema
    RelationalSchemaObject GetSchema() const;

private:
    RelationalSchemaObject schema_;
};
}
#endif
#endif