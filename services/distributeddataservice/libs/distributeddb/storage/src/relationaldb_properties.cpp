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
#ifdef RELATIONAL_STORE
#include "relationaldb_properties.h"

namespace DistributedDB {
const std::string RelationalDBProperties::DISTRIBUTED_TABLE_MODE = "distributed_table_mode";

RelationalDBProperties::RelationalDBProperties()
    : schema_(),
      isEncrypted_(false),
      cipherType_(),
      passwd_(),
      iterTimes_(0)
{}

RelationalDBProperties::~RelationalDBProperties()
{}

bool RelationalDBProperties::IsSchemaExist() const
{
    return schema_.IsSchemaValid();
}

void RelationalDBProperties::SetSchema(const RelationalSchemaObject &schema)
{
    schema_ = schema;
}

RelationalSchemaObject RelationalDBProperties::GetSchema() const
{
    return schema_;
}

void RelationalDBProperties::SetCipherArgs(CipherType cipherType, const CipherPassword &passwd, uint32_t iterTimes)
{
    isEncrypted_ = true;
    cipherType_ = cipherType;
    passwd_ = passwd;
    iterTimes_ = iterTimes;
}

bool RelationalDBProperties::IsEncrypted() const
{
    return isEncrypted_;
}

CipherType RelationalDBProperties::GetCipherType() const
{
    return cipherType_;
}

const CipherPassword &RelationalDBProperties::GetPasswd() const
{
    return passwd_;
}

uint32_t RelationalDBProperties::GetIterTimes() const
{
    return iterTimes_;
}
}
#endif