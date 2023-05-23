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
#include "document_key.h"

#include "doc_errno.h"
#include "log_print.h"
#include "securec.h"
namespace DocumentDB {

static uint16_t g_oIdIncNum = 0;

static int InitDocIdFromOid(DocKey &docKey)
{
    time_t nowTime = time(nullptr);
    if (nowTime < 0) {
        return -E_INNER_ERROR;
    }
    uint32_t now = (uint32_t)nowTime;
    uint16_t iv = g_oIdIncNum++;
    // The maximum number of autoincrements is 65535, and if it is exceeded, it becomes 0.
    if (g_oIdIncNum > (uint16_t)65535) {
        g_oIdIncNum = (uint16_t)0;
    }
    char *idTemp = new char[GRD_DOC_OID_HEX_SIZE + 1];
    if (sprintf_s(idTemp, GRD_DOC_OID_HEX_SIZE + 1, "%08x%04x", now, iv) < 0) {
        GLOGE("get oid error");
        return -E_INNER_ERROR;
    }
    docKey.id = idTemp;
    delete[] idTemp;
    docKey.type = (uint8_t)STRING;
    return E_OK;
}

static int SerializeDocKey(DocKey &key, const std::string &id, const int32_t size)
{
    std::string idStr = id;
    key.key = idStr;
    key.key = key.key + std::to_string(key.type); // Question here
    key.keySize = size + GRD_DOC_ID_TYPE_SIZE;
    return E_OK;
}

int DocumentKey::GetOidDocKey(DocKey &key)
{
    int ret = InitDocIdFromOid(key);
    {
        if (ret != E_OK) {
            return ret;
        }
    }
    ret = SerializeDocKey(key, key.id, GRD_DOC_OID_HEX_SIZE);
    return ret;
}

std::string DocumentKey::GetIdFromKey(std::string &keyStr)
{
    if (!keyStr.empty()) {
        keyStr.pop_back();
    }
    return keyStr;
}
} // namespace DocumentDB