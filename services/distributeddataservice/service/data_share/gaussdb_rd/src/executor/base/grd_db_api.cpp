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

#include "grd_base/grd_db_api.h"

#include "grd_base/grd_error.h"

int32_t GRD_DBOpen(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db)
{
    return GRD_OK;
}

int32_t GRD_DBClose(GRD_DB *db, uint32_t flags)
{
    return GRD_OK;
}

int32_t GRD_Flush(GRD_DB *db, uint32_t flags)
{
    return GRD_OK;
}