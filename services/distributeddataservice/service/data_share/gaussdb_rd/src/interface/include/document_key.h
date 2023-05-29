/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DOCUMENT_KEY_H
#define DOCUMENT_KEY_H

#include <stdint.h>
#include <string>

#include "json_object.h"

#define GRD_DOC_OID_TIME_SIZE 4
#define GRD_DOC_OID_INCREMENTAL_VALUE_SIZE 2
#define GRD_DOC_OID_SIZE (GRD_DOC_OID_TIME_SIZE + GRD_DOC_OID_INCREMENTAL_VALUE_SIZE)
#define GRD_DOC_OID_HEX_SIZE (GRD_DOC_OID_SIZE * 2)
#define GRD_DOC_ID_TYPE_SIZE 1

namespace DocumentDB {
enum class DocIdType {
    INT = 1,
    STRING,
};

struct DocKey {
public:
    ~DocKey(){};
    int32_t keySize;
    std::string key;
    uint8_t type;
};

class DocumentKey {
public:
    static int GetOidDocKey(DocKey &key);
    static int GetStringDocKey(const std::string &id, DocKey &key);
    static std::string GetIdFromKey(std::string &keyStr);
};
} // namespace DocumentDB
#endif // DOCUMENT_KEY_H