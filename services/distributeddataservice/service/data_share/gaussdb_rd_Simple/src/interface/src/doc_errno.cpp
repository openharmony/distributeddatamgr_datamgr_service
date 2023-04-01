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
int TrasnferDocErr(int err)
{
    switch (err) {
        case E_OK:
            return GRD_OK;
        case -E_ERROR:
            return GRD_INNER_ERR;
        case -E_INVALID_ARGS:
            return GRD_INVALID_ARGS;
        case -E_FILE_OPERATION:
            return GRD_FAILED_FILE_OPERATION;
        case -E_OVER_LIMIT:
            return GRD_OVER_LIMIT;
        case -E_INVALID_JSON_FORMAT:
            return GRD_INVALID_JSON_FORMAT;
        case -E_INVALID_CONFIG_VALUE:
            return GRD_INVALID_CONFIG_VALUE;
        case -E_COLLECTION_CONFLICT:
            return GRD_COLLECTION_CONFLICT;
        case -E_NO_DATA:
            return GRD_NO_DATA;
        default:
            return GRD_INNER_ERR;
    }
}
}