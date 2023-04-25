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

#include "doc_errno.h"
#include "grd_base/grd_error.h"

namespace DocumentDB {
int GetErrorCategory(int errCode)
{
    int categoryCode = errCode % 1000000;
    categoryCode /= 1000;
    categoryCode *= 1000;
    return categoryCode;
}

int TrasnferDocErr(int err)
{
    if (err > 0) {
        return err;
    }

    int outErr = GRD_OK;
    switch (err) {
        case E_OK:
            return GRD_OK;
        case -E_ERROR:
            outErr = GRD_INNER_ERR;
            break;
        case -E_INVALID_ARGS:
            outErr = GRD_INVALID_ARGS;
            break;
        case -E_FILE_OPERATION:
            outErr = GRD_FAILED_FILE_OPERATION;
            break;
        case -E_OVER_LIMIT:
            outErr = GRD_OVER_LIMIT;
            break;
        case -E_INVALID_JSON_FORMAT:
            outErr = GRD_INVALID_JSON_FORMAT;
            break;
        case -E_INVALID_CONFIG_VALUE:
            outErr = GRD_INVALID_CONFIG_VALUE;
            break;
        case -E_DATA_CONFLICT:
            outErr = GRD_DATA_CONFLICT;
            break;
        case -E_COLLECTION_CONFLICT:
            outErr = GRD_COLLECTION_CONFLICT;
            break;
        case -E_NO_DATA:
        case -E_NOT_FOUND:
            outErr = GRD_NO_DATA;
            break;
        case -E_INVALID_COLL_NAME_FORMAT:
            outErr = GRD_INVALID_COLLECTION_NAME;
            break;
        case -E_RESOURCE_BUSY:
            outErr = GRD_RESOURCE_BUSY;
            break;
        case -E_FAILED_MEMORY_ALLOCATE:
            outErr = GRD_FAILED_MEMORY_ALLOCATE;
            break;
        default:
            outErr = GRD_INNER_ERR;
            break;
    }

    return GetErrorCategory(outErr);
}
}