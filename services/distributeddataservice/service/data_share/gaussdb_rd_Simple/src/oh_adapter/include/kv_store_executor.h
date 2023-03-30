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

#ifndef KV_STORE_EXECUTOR_H
#define KV_STORE_EXECUTOR_H

#include <string>
#include "doc_common.h"

namespace DocumentDB {
class KvStoreExecutor {
public:
    virtual ~KvStoreExecutor() = default;

    virtual int PutData(const std::string &collName, const Key &key, const Value &value) = 0;

    virtual int GetData(const std::string &collName, const Key &key, Value &value) const = 0;

    virtual int CreateCollection(const std::string &name, bool ignoreExists) = 0;
    virtual int DropCollection(const std::string &name, bool ignoreNotExists) = 0;

    virtual int GetCollectionOption(const std::string &name, std::string &option) = 0;
    virtual int SetCollectionOption(const std::string &name, const std::string &option) = 0;
};
} // DocumentDB
#endif // KV_STORE_EXECUTOR_H