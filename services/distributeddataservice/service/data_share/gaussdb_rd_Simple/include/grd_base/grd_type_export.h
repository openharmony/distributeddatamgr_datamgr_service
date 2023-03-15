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

#ifndef GRD_TYPE_EXPORT_H
#define GRD_TYPE_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct GRD_DB GRD_DB;

/**
 * @brief Open database config
 */
#define GRD_DB_OPEN_ONLY        0x00
#define GRD_DB_OPEN_CREATE      0x01

/**
 * @brief Close database config
 */
#define GRD_DB_CLOSE                0x00
#define GRD_DB_CLOSE_IGNORE_ERROR   0x01

#define GRD_DB_ID_DISPLAY       0x01

typedef struct Query {
    const char *filter;
    const char *projection;
} Query;

/**
 * @brief Flags for create and drop collection
 */
#define FLAG_CHECK_UDEFINED_DUPLICAte_TABLE 1

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // GRD_TYPE_EXPORT_H