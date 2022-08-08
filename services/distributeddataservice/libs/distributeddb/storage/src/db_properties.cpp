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

#include "db_common.h"
#include "db_properties.h"

namespace DistributedDB {
const std::string DBProperties::CREATE_IF_NECESSARY = "createIfNecessary";
const std::string DBProperties::DATABASE_TYPE = "databaseType";
const std::string DBProperties::DATA_DIR = "dataDir";
const std::string DBProperties::USER_ID = "userId";
const std::string DBProperties::APP_ID = "appId";
const std::string DBProperties::STORE_ID = "storeId";
const std::string DBProperties::INSTANCE_ID = "instanceId";
const std::string DBProperties::IDENTIFIER_DATA = "identifier";
const std::string DBProperties::IDENTIFIER_DIR = "identifierDir";
const std::string DBProperties::DUAL_TUPLE_IDENTIFIER_DATA = "dualTupleIdentifier";
const std::string DBProperties::SYNC_DUAL_TUPLE_MODE = "syncDualTuple";

std::string DBProperties::GetStringProp(const std::string &name, const std::string &defaultValue) const
{
    auto iter = stringProperties_.find(name);
    if (iter != stringProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetStringProp(const std::string &name, const std::string &value)
{
    stringProperties_[name] = value;
}

bool DBProperties::GetBoolProp(const std::string &name, bool defaultValue) const
{
    auto iter = boolProperties_.find(name);
    if (iter != boolProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetBoolProp(const std::string &name, bool value)
{
    boolProperties_[name] = value;
}

int DBProperties::GetIntProp(const std::string &name, int defaultValue) const
{
    auto iter = intProperties_.find(name);
    if (iter != intProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetIntProp(const std::string &name, int value)
{
    intProperties_[name] = value;
}

void DBProperties::SetIdentifier(const std::string &userId, const std::string &appId, const std::string &storeId,
    int32_t instanceId)
{
    SetStringProp(DBProperties::APP_ID, appId);
    SetStringProp(DBProperties::USER_ID, userId);
    SetStringProp(DBProperties::STORE_ID, storeId);
    SetIntProp(DBProperties::INSTANCE_ID, instanceId);
    std::string hashIdentifier = DBCommon::TransferHashString(
        DBCommon::GenerateIdentifierId(storeId, appId, userId, instanceId));
    SetStringProp(DBProperties::IDENTIFIER_DATA, hashIdentifier);
    std::string dualIdentifier = DBCommon::TransferHashString(DBCommon::GenerateDualTupleIdentifierId(storeId, appId));
    SetStringProp(DBProperties::DUAL_TUPLE_IDENTIFIER_DATA, dualIdentifier);
}
}