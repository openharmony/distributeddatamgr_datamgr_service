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
#ifndef RELATIONAL_STORE_EXT_H
#define RELATIONAL_STORE_EXT_H
#ifdef RELATIONAL_STORE

#define SQLITE3_HW_EXPORT_SYMBOLS

#include "sqlite3.h"
#include "types.h"

// We extend the original purpose of the "sqlite3ext.h".
struct sqlite3_api_routines_relational {
    int (*close)(sqlite3*);
    int (*close_v2)(sqlite3*);
    int (*open)(const char *, sqlite3 **);
    int (*open16)(const void *, sqlite3 **);
    int (*open_v2)(const char *, sqlite3 **, int, const char *);
};

extern const struct sqlite3_api_routines_relational *sqlite3_export_relational_symbols;
#define sqlite3_close           sqlite3_export_relational_symbols->close
#define sqlite3_close_v2        sqlite3_export_relational_symbols->close_v2
#define sqlite3_open            sqlite3_export_relational_symbols->open
#define sqlite3_open16          sqlite3_export_relational_symbols->open16
#define sqlite3_open_v2         sqlite3_export_relational_symbols->open_v2

#endif
#endif // RELATIONAL_STORE_EXT_H