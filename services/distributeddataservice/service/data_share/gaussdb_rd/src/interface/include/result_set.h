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

#ifndef RESULTSET_H
#define RESULTSET_H

#include <string>
#include <vector>

#include "check_common.h"
#include "doc_errno.h"
#include "document_store.h"
#include "grd_base/grd_type_export.h"
#include "json_object.h"

namespace DocumentDB {
class ResultSet {
public:
    ResultSet();
    ~ResultSet();
    int Init(std::shared_ptr<QueryContext> &context, DocumentStore *store, bool isCutBranch);
    int GetNext(bool isNeedTransaction = false, bool isNeedCheckTable = false);
    int GetValue(char **value);
    int GetValue(std::string &value);
    int GetKey(std::string &key);
    int EraseCollection();

private:
    int GetNextInner(bool isNeedCheckTable);
    int CutJsonBranch(std::string &jsonData);
    int CheckCutNode(JsonObject *node, std::vector<std::string> singleCutPath,
        std::vector<std::vector<std::string>> &allCutPath);
    int GetNextWithField();

    DocumentStore *store_ = nullptr;
    bool isCutBranch_ = false;
    size_t index_ = 0;
    std::shared_ptr<QueryContext> context_;
    std::pair<std::string, std::string> matchData_;
};
} // namespace DocumentDB
#endif // RESULTSET_H