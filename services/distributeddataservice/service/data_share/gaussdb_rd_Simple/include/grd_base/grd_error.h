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

#ifndef GRD_ERROR_H
#define GRD_ERROR_H

#include "grd_type_export.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define GRD_OK 0

#define GRD_NOT_SUPPORT (-1000)
#define GRD_OVER_LIMIT (-2000)
#define GRD_INVALID_ARGS (-3000)
#define GRD_SYSTEM_ERR (-4000)
#define GRD_NO_DATA (-11000)
#define GRD_FAILED_MEMORY_ALLOCATE (-13000)
#define GRD_FAILED_MEMORY_RELEASE (-14000)
#define GRD_INVALID_FORMAT (-37000)

#define GRD_FIELD_NOT_FOUND (-5003002)
#define GRD_FIELD_TYPE_NOT_MATCH (-5003003)
#define GRD_LARGE_JSON_NEST (-5003004)

#define GRD_UNVAILABLE_JSON_LIB (-5004001)


#define GRD_COLLECTION_NOT_FOUND (-5011001)
#define GRD_RECORD_NOT_FOUND (-5011002)

#define GRD_INVALID_JSON_FORMAT (-5037001)

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // GRD_ERROR_H