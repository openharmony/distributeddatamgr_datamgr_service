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

#ifndef GRD_DOCUMENT_API_H
#define GRD_DOCUMENT_API_H

#include "grd_base/grd_type_export.h"
#include "grd_base/grd_resultset_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int GRD_CreateCollection(GRD_DB *db, const char *collectionName, const char *optionStr, unsigned int flags);

int GRD_DropCollection(GRD_DB *db, const char *collectionName, unsigned int flags);

int GRD_InsertDoc(GRD_DB *db, const char *collectionName, const char *document, unsigned int flags);

int GRD_FindDoc(GRD_DB *db, const char *collectionName, Query query, unsigned int flags, GRD_ResultSet **resultSet);

int GRD_UpdateDoc(GRD_DB *db, const char *collectionName, const char *filter, const char *update, unsigned int flags);

int GRD_UpsertDoc(GRD_DB *db, const char *collectionName, const char *filter, const char *document, unsigned int flags);

int GRD_DeleteDoc(GRD_DB *db, const char *collectionName, const char *filter, unsigned int flags);

int GRD_Flush(GRD_DB *db, unsigned int flags);

#ifdef __cplusplus
}
#endif
#endif // GRD_DOCUMENT_API_H