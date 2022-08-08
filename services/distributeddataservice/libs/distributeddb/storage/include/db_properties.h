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
#ifndef DB_PROPERTIES_H
#define DB_PROPERTIES_H

#include <map>
#include <string>

namespace DistributedDB {
class DBProperties {
public:
    // Get the string property according the name
    std::string GetStringProp(const std::string &name, const std::string &defaultValue) const;

    // Set the string property for the name
    void SetStringProp(const std::string &name, const std::string &value);

    // Get the bool property according the name
    bool GetBoolProp(const std::string &name, bool defaultValue) const;

    // Set the bool property for the name
    void SetBoolProp(const std::string &name, bool value);

    // Get the bool property according the name
    int GetIntProp(const std::string &name, int defaultValue) const;

    // Set the integer property for the name
    void SetIntProp(const std::string &name, int value);

    // Set all indentifers
    void SetIdentifier(const std::string &userId, const std::string &appId, const std::string &storeId,
        int32_t instanceId = 0);

    static const std::string CREATE_IF_NECESSARY;
    static const std::string DATABASE_TYPE;
    static const std::string DATA_DIR;
    static const std::string USER_ID;
    static const std::string APP_ID;
    static const std::string STORE_ID;
    static const std::string INSTANCE_ID;
    static const std::string IDENTIFIER_DATA;
    static const std::string IDENTIFIER_DIR;
    static const std::string DUAL_TUPLE_IDENTIFIER_DATA;
    static const std::string SYNC_DUAL_TUPLE_MODE;

protected:
    DBProperties() = default;
    virtual ~DBProperties() = default;

    std::map<std::string, std::string> stringProperties_;
    std::map<std::string, bool> boolProperties_;
    std::map<std::string, int> intProperties_;
};
}
#endif