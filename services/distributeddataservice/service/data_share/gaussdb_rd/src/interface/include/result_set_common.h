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

#ifndef RESULTSET_COMMON_H
#define RESULTSET_COMMON_H

#include <string>

#include "doc_errno.h"
#include "grd_base/grd_type_export.h"
#include "result_set.h"
#include "vector"

namespace DocumentDB {
class ValueObject;
int InitResultSet(std::shared_ptr<QueryContext> &context, DocumentStore *store, ResultSet &resultSet, bool isCutBranch);
} // namespace DocumentDB
#endif // RESULTSET_COMMON_H
