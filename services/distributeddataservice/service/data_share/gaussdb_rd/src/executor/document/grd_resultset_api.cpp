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
#include "grd_base/grd_resultset_api.h"

#include <mutex>

#include "doc_errno.h"
#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "log_print.h"

using namespace DocumentDB;

int32_t GRD_Next(GRD_ResultSet *resultSet)
{
    if (resultSet == nullptr) {
        GLOGE("resultSet is nullptr");
        return GRD_INVALID_ARGS;
    };
    int ret = resultSet->resultSet_.GetNext(true, true);
    return TransferDocErr(ret);
}

int32_t GRD_GetValue(GRD_ResultSet *resultSet, char **value)
{
    if (resultSet == nullptr || value == nullptr) {
        GLOGE("resultSet is nullptr,cant get value from it");
        return GRD_INVALID_ARGS;
    };
    char *val = nullptr;
    int ret = resultSet->resultSet_.GetValue(&val);
    if (val == nullptr) {
        GLOGE("Value that get from resultSet is nullptr");
        return GRD_NOT_AVAILABLE;
    }
    *value = val;
    return TransferDocErr(ret);
}

int32_t GRD_FreeValue(char *value)
{
    if (value == nullptr) {
        return GRD_INVALID_ARGS;
    }
    delete[] value;
    return GRD_OK;
}

int32_t GRD_FreeResultSet(GRD_ResultSet *resultSet)
{
    if (resultSet == nullptr) {
        return GRD_INVALID_ARGS;
    }
    resultSet->resultSet_.EraseCollection();
    delete resultSet;
    return GRD_OK;
}