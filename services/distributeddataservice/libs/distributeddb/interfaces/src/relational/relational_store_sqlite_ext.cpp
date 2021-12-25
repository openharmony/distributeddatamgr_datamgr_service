/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifdef RELATIONAL_STORE
#include "sqlite_utils.h"
#include "log_print.h"
using namespace DistributedDB;
// namespace DistributedDB {
// We extend the original purpose of the "sqlite3ext.h".
SQLITE_API int sqlite3_close_relational(sqlite3* db)
{
    ::LOGD("### my sqlite3_close_relational!");
    return sqlite3_close(db);
}

SQLITE_API int sqlite3_close_v2_relational(sqlite3* db)
{
    ::LOGD("### my sqlite3_close_v2_relational!");
    return sqlite3_close_v2(db);
}

SQLITE_API int sqlite3_open_relational(const char *filename, sqlite3 **ppDb)
{
    int err = sqlite3_open(filename, ppDb);
    DistributedDB::SQLiteUtils::RegisterCalcHash(*ppDb);
    DistributedDB::SQLiteUtils::RegisterGetSysTime(*ppDb);
    ::LOGD("### my sqlite3_open!");
    return err;
}

SQLITE_API int sqlite3_open16_relational(const void *filename, sqlite3 **ppDb)
{
    int err = sqlite3_open16(filename, ppDb);
    DistributedDB::SQLiteUtils::RegisterCalcHash(*ppDb);
    DistributedDB::SQLiteUtils::RegisterGetSysTime(*ppDb);
    ::LOGD("### my sqlite3_open16!");
    return err;
}

SQLITE_API int sqlite3_open_v2_relational(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
{
    int err = sqlite3_open_v2(filename, ppDb, flags, zVfs);
    DistributedDB::SQLiteUtils::RegisterCalcHash(*ppDb);
    DistributedDB::SQLiteUtils::RegisterGetSysTime(*ppDb);
    ::LOGD("### my sqlite3_open_v2!");
    return err;
}

// hw export the symbols
#ifdef SQLITE_DISTRIBUTE_RELATIONAL
#if defined(__GNUC__)
#  define EXPORT_SYMBOLS  __attribute__ ((visibility ("default")))
#elif defined(_MSC_VER)
    #  define EXPORT_SYMBOLS  __declspec(dllexport)
#else
#  define EXPORT_SYMBOLS
#endif

struct sqlite3_api_routines_relational {
    int (*close)(sqlite3*);
    int (*close_v2)(sqlite3*);
    int (*open)(const char *, sqlite3 **);
    int (*open16)(const void *, sqlite3 **);
    int (*open_v2)(const char *, sqlite3 **, int, const char *);
};

typedef struct sqlite3_api_routines_relational sqlite3_api_routines_relational;
static const sqlite3_api_routines_relational sqlite3HwApis = {
#ifdef SQLITE_DISTRIBUTE_RELATIONAL
    sqlite3_close_relational,
    sqlite3_close_v2_relational,
    sqlite3_open_relational,
    sqlite3_open16_relational,
    sqlite3_open_v2_relational
#else
    0,
    0,
    0,
    0,
    0
#endif
};

EXPORT_SYMBOLS const sqlite3_api_routines_relational *sqlite3_export_relational_symbols = &sqlite3HwApis;
#endif

#endif