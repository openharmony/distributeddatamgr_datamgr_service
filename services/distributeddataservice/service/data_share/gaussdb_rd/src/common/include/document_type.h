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

#ifndef DOCUMENT_TYPE_H
#define DOCUMENT_TYPE_H

#include <string>

#include "projection_tree.h"

namespace DocumentDB {
struct QueryContext {
    std::string collectionName;
    std::string filter;
    std::vector<std::vector<std::string>> projectionPath;
    ProjectionTree projectionTree;
    bool ifShowId = false;
    bool viewType = false;
    bool isIdExist = false;
};
} // namespace DocumentDB
#endif // DOCUMENT_TYPE_H