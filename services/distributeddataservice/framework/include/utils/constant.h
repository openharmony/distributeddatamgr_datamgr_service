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

#ifndef KV_DATASERVICE_CONSTANT_H
#define KV_DATASERVICE_CONSTANT_H

#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <cctype>
#include <locale>
#include <string>
#include <vector>
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
class KvStoreMetaRow {
public:
    API_EXPORT static const std::string KEY_PREFIX;

    API_EXPORT static std::vector<uint8_t> GetKeyFor(const std::string &key);
};

class SecretMetaRow {
public:
    API_EXPORT static const std::string KEY_PREFIX;

    API_EXPORT static std::vector<uint8_t> GetKeyFor(const std::string &key);
};

class Constant {
public:
    // concatenate strings and return a composition string.
    API_EXPORT static std::string Concatenate(std::initializer_list<std::string> stringList);

    // delete left bland in s by reference.
    template<typename T>
    static void LeftTrim(T &s);

    // delete right bland in s by reference.
    template<typename T>
    static void RightTrim(T &s);

    // delete both left and right bland in s by reference.
    template<typename T>
    static void Trim(T &s);

    // delete left bland in s by reference, not change raw string.
    template<typename T>
    static T LeftTrimCopy(T s);

    // delete right bland in s by reference, not change raw string.
    template<typename T>
    static T RightTrimCopy(T s);

    // delete both left and right bland in s by reference, not change raw string.
    template<typename T>
    static T TrimCopy(T s);

    // get default device account id.
    API_EXPORT static std::string GetDefaultDeviceAccountId();

    // get default harmony account name.
    API_EXPORT static std::string GetDefaultHarmonyAccountName();

    // default group id for synchronization based on harmony account.
    API_EXPORT static const std::string DEFAULT_GROUP_ID;

    // Indicates whether only storeid are used as hash materials for the DistributedDB path generated.
    API_EXPORT static const bool STOREID_ONLY_FLAG;

    // version for distributed kv data service.
    API_EXPORT static const std::string VERSION;

    // meta name for distributed kv data service.
    API_EXPORT static const std::string META_DIR_NAME;

    // name for distributed kv data service.
    API_EXPORT static const std::string SERVICE_NAME;

    // root path for distributed kv data service.
    API_EXPORT static const std::string ROOT_PATH;

    // root path for distributeddata service and system services.
    API_EXPORT static const std::string ROOT_PATH_DE;

    // root path for self-developed and non-self-developed app.
    API_EXPORT static const std::string ROOT_PATH_CE;

    // the max length for key is 256.
    API_EXPORT static const size_t MAX_KEY_LENGTH;

    // the max length for value is 1M.
    API_EXPORT static const size_t MAX_VALUE_LENGTH;

    // the max length for StoreId is 64.
    API_EXPORT static const size_t MAX_STORE_ID_LENGTH;

    // the max batch for putBatch is 128.
    API_EXPORT static const size_t MAX_BATCH_SIZE;

    // the max capacity for ipc is 800KB.
    API_EXPORT static const size_t MAX_IPC_CAPACITY;

    // service meta db name.
    API_EXPORT static const std::string SERVICE_META_DB_NAME;

    API_EXPORT static const std::string KEY_SEPARATOR;

    API_EXPORT static const mode_t DEFAULT_MODE;

    API_EXPORT static const mode_t DEFAULT_MODE_DIR;

    API_EXPORT static const mode_t DEFAULT_MODE_FILE;

    API_EXPORT static const int SWITCH_RAW_DATA_SIZE;

    API_EXPORT static const int MAX_OPEN_KVSTORES;

    // name for process label (bus name for communication). compatible with HwDDMP
    API_EXPORT static const std::string ROOT_KEY_GENERATED;
};

// trim from start (in place)
template<typename T>
void Constant::LeftTrim(T &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
template<typename T>
void Constant::RightTrim(T &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
template<typename T>
void Constant::Trim(T &s)
{
    LeftTrim(s);
    RightTrim(s);
}

// trim from start (copying)
template<typename T>
T Constant::LeftTrimCopy(T s)
{
    LeftTrim(s);
    return s;
}

// trim from end (copying)
template<typename T>
T Constant::RightTrimCopy(T s)
{
    RightTrim(s);
    return s;
}

// trim from both ends (copying)
template<typename T>
T Constant::TrimCopy(T s)
{
    Trim(s);
    return s;
}
}  // namespace DistributedKv
}  // namespace OHOS
#endif // KV_DATASERVICE_CONSTANT_H
