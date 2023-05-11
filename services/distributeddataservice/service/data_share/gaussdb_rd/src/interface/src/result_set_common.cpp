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

#include "result_set_common.h"

#include <string>
#include <vector>

#include "doc_errno.h"
#include "grd_base/grd_error.h"

namespace DocumentDB {
class ValueObject;
int InitResultSet(DocumentStore *store, const std::string collectionName, const std::string &filter,
    std::vector<std::vector<std::string>> &path, bool ifShowId, bool viewType, ResultSet &resultSet, bool &isOnlyId)
{
    return resultSet.Init(store, collectionName, filter, path, ifShowId, viewType, isOnlyId);
}

int InitResultSet(DocumentStore *store, const std::string collectionName, const std::string &filter,
    ResultSet &resultSet)
{
    return resultSet.Init(store, collectionName, filter);
}
} // namespace DocumentDB
