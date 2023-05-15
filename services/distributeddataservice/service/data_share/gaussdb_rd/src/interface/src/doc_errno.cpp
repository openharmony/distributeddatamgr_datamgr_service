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
    int categoryCode = errCode % 1000000; // 1000000: mod to get last 6 digits
    categoryCode /= 1000;                 // 1000: deviced to remove first 3 digits
    categoryCode *= 1000;                 // 1000: multiply to pad the output
    return categoryCode;
}

int TransferDocErr(int err)
{
    if (err > 0) {
        return err;
    }

    switch (err) {
        case E_OK:
            return GRD_OK;
        case -E_ERROR:
            return GetErrorCategory(GRD_INNER_ERR);
        case -E_INVALID_ARGS:
            return GetErrorCategory(GRD_INVALID_ARGS);
        case -E_FILE_OPERATION:
            return GetErrorCategory(GRD_FAILED_FILE_OPERATION);
        case -E_OVER_LIMIT:
            return GetErrorCategory(GRD_OVER_LIMIT);
        case -E_INVALID_JSON_FORMAT:
            return GetErrorCategory(GRD_INVALID_JSON_FORMAT);
        case -E_INVALID_CONFIG_VALUE:
            return GetErrorCategory(GRD_INVALID_CONFIG_VALUE);
        case -E_DATA_CONFLICT:
            return GetErrorCategory(GRD_DATA_CONFLICT);
        case -E_COLLECTION_CONFLICT:
            return GetErrorCategory(GRD_COLLECTION_CONFLICT);
        case -E_NO_DATA:
        case -E_NOT_FOUND:
            return GetErrorCategory(GRD_NO_DATA);
        case -E_INVALID_COLL_NAME_FORMAT:
            return GetErrorCategory(GRD_INVALID_COLLECTION_NAME);
        case -E_RESOURCE_BUSY:
            return GetErrorCategory(GRD_RESOURCE_BUSY);
        case -E_FAILED_MEMORY_ALLOCATE:
        case -E_OUT_OF_MEMORY:
            return GetErrorCategory(GRD_FAILED_MEMORY_ALLOCATE);
        case -E_INVALID_FILE_FORMAT:
            return GetErrorCategory(GRD_INVALID_FILE_FORMAT);
        case -E_INNER_ERROR:
        default:
            return GetErrorCategory(GRD_INNER_ERR);
    }
}
} // namespace DocumentDB