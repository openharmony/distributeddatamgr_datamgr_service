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

#ifndef _WIN32
    #define DOC_API __attribute__ ((visibility ("default")))
#endif

typedef struct GRD_DB GRD_DB;

/**
 * @brief Open database config
 */
#define GRD_DB_OPEN_ONLY        0x00
#define GRD_DB_OPEN_CREATE      0x01
#define GRD_DB_OPEN_CHECK_FOR_ABNORMAL      0x02 //check data in database if close abnormally last time,
                                                 //if data is corrupted, rebuild the database
#define GRD_DB_OPEN_CHECK       0x04//check data in database when open database, if data is corrupted, rebuild the database.
/**
 * @brief Close database config
 */
#define GRD_DB_CLOSE                0x00
#define GRD_DB_CLOSE_IGNORE_ERROR   0x01

/**
 * @brief flush database config
 */
#define GRD_DB_FLUSH_ASYNC 0x00
#define GRD_DB_FLUSH_SYNC 0x01

#define GRD_DOC_ID_DISPLAY      0x01
typedef struct Query {
    const char *filter;
    const char *projection;
} Query;

/**
 * @brief Flags for create and drop collection
 */
#define CHK_EXIST_COLLECTION 1
#define CHK_NON_EXIST_COLLECTION 1

#define GRD_DOC_APPEND 0
#define GRD_DOC_REPLACE 1

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // GRD_TYPE_EXPORT_H