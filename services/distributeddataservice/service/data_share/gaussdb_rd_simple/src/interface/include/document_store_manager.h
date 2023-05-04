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

#ifndef DOCUMENT_STORE_MANAGER_H
#define DOCUMENT_STORE_MANAGER_H

#include <string>

#include "document_store.h"

namespace DocumentDB {
class DocumentStoreManager {
public:
    static int GetDocumentStore(
        const std::string &path, const std::string &config, unsigned int flags, DocumentStore *&store);

    static int CloseDocumentStore(DocumentStore *store, unsigned int flags);

private:
    static bool CheckDBPath(const std::string &path, std::string &canonicalPath, std::string &dbName, int &errCode);
    static bool CheckDBConfig(const std::string &config, int &errCode);
};
} // namespace DocumentDB
#endif // DOCUMENT_STORE_MANAGER_H