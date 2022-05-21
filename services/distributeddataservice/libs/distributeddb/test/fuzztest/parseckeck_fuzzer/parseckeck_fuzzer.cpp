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

#include "parseckeck_fuzzer.h"
#include "distributeddb_tools_test.h"
#include "schema_object.h"
#include "schema_utils.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

static KvStoreConfig g_config;

namespace OHOS {
void GetSchmaKvstore(const uint8_t* data, size_t size)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    kvManager.SetKvStoreConfig(g_config);
    KvStoreNbDelegate::Option option = {true, false, false};
    std::string schemaString(reinterpret_cast<const char *>(data), size);
    option.schema = schemaString;
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    kvManager.GetKvStore("distributed_nb_get_schemakvstore", option,
        [&kvNbDelegatePtr] (DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });

    kvManager.CloseKvStore(kvNbDelegatePtr);
    kvManager.DeleteKvStore("distributed_nb_get_schemakvstore");
}

void ParseSchemaString(const uint8_t* data, size_t size)
{
    std::string schemaString(reinterpret_cast<const char *>(data), size);
    SchemaObject schemaOri;
    schemaOri.ParseFromSchemaString(schemaString);
    schemaOri.CompareAgainstSchemaString(schemaString);
}

void CompareSchemaString(const uint8_t* data, size_t size)
{
    // beginning half / 2
    std::string schemaString(data, data + (size / 2));
    // ending half / 2 ~ end.
    std::string schemaString2(data + (size / 2), data + size);
    SchemaObject schemaOri;
    schemaOri.ParseFromSchemaString(schemaString);
    schemaOri.ParseFromSchemaString(schemaString2);
}

void CheckFieldName(const uint8_t* data, size_t size)
{
    std::string schemaAttrString(reinterpret_cast<const char *>(data), size);
    SchemaUtils::CheckFieldName(schemaAttrString);
}

void ParseFieldPath(const uint8_t* data, size_t size)
{
    std::string schemaAttrString(reinterpret_cast<const char *>(data), size);
    FieldPath outPath;
    SchemaUtils::ParseAndCheckFieldPath(schemaAttrString, outPath);
}

void CheckSchemaAttribute(const uint8_t* data, size_t size)
{
    std::string schemaAttrString(reinterpret_cast<const char *>(data), size);
    SchemaAttribute outAttr;
    SchemaUtils::ParseAndCheckSchemaAttribute(schemaAttrString, outAttr);
    SchemaUtils::ParseAndCheckSchemaAttribute(schemaAttrString, outAttr);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    DistributedDBToolsTest::TestDirInit(g_config.dataDir);
    OHOS::GetSchmaKvstore(data, size);
    OHOS::ParseSchemaString(data, size);
    OHOS::CompareSchemaString(data, size);
    OHOS::CheckFieldName(data, size);
    OHOS::ParseFieldPath(data, size);
    OHOS::CheckSchemaAttribute(data, size);

    DistributedDBToolsTest::RemoveTestDbFiles(g_config.dataDir);
    return 0;
}

