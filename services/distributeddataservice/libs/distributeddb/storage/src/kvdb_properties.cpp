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

#include "kvdb_properties.h"

#include "db_constant.h"

namespace DistributedDB {
const std::string KvDBProperties::FILE_NAME = "fileName";
const std::string KvDBProperties::MEMORY_MODE = "memoryMode";
const std::string KvDBProperties::ENCRYPTED_MODE = "isEncryptedDb";
const std::string KvDBProperties::FIRST_OPEN_IS_READ_ONLY = "firstOpenIsReadOnly";
const std::string KvDBProperties::CREATE_DIR_BY_STORE_ID_ONLY = "createDirByStoreIdOnly";
const std::string KvDBProperties::SECURITY_LABEL = "securityLabel";
const std::string KvDBProperties::SECURITY_FLAG = "securityFlag";
const std::string KvDBProperties::CONFLICT_RESOLVE_POLICY = "conflictResolvePolicy";
const std::string KvDBProperties::CHECK_INTEGRITY = "checkIntegrity";
const std::string KvDBProperties::RM_CORRUPTED_DB = "rmCorruptedDb";
const std::string KvDBProperties::COMPRESS_ON_SYNC = "needCompressOnSync";
const std::string KvDBProperties::COMPRESSION_RATE = "compressionRate";

KvDBProperties::KvDBProperties()
    : cipherType_(CipherType::AES_256_GCM)
{}

KvDBProperties::~KvDBProperties() {}

std::string KvDBProperties::GetStoreSubDirectory(int type)
{
    switch (type) {
        case LOCAL_TYPE:
            return DBConstant::LOCAL_SUB_DIR;
        case MULTI_VER_TYPE:
            return DBConstant::MULTI_SUB_DIR;
        case SINGLE_VER_TYPE:
            return DBConstant::SINGLE_SUB_DIR;
        default:
            return "unknown";
    }
}

void KvDBProperties::GetPassword(CipherType &type, CipherPassword &password) const
{
    type = cipherType_;
    password = password_;
}

void KvDBProperties::SetPassword(CipherType type, const CipherPassword &password)
{
    cipherType_ = type;
    password_ = password;
}

bool KvDBProperties::IsSchemaExist() const
{
    return schema_.IsSchemaValid();
}

void KvDBProperties::SetSchema(const SchemaObject &schema)
{
    schema_ = schema;
}

SchemaObject KvDBProperties::GetSchema() const
{
    return schema_;
}

int KvDBProperties::GetSecLabel() const
{
    return GetIntProp(KvDBProperties::SECURITY_LABEL, 0);
}

int KvDBProperties::GetSecFlag() const
{
    return GetIntProp(KvDBProperties::SECURITY_FLAG, 0);
}

const SchemaObject &KvDBProperties::GetSchemaConstRef() const
{
    return schema_;
}
} // namespace DistributedDB
