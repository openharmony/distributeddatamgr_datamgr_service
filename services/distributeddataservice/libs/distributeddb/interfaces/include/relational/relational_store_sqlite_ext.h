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

#define SQLITE3_HW_EXPORT_SYMBOLS

// using the "sqlite3sym.h" in OHOS
#ifndef USE_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif

// We extend the original purpose of the "sqlite3ext.h".
struct sqlite3_api_routines_relational {
    int (*open)(const char *, sqlite3 **);
    int (*open16)(const void *, sqlite3 **);
    int (*open_v2)(const char *, sqlite3 **, int, const char *);
};

extern const struct sqlite3_api_routines_relational *sqlite3_export_relational_symbols;

#ifdef sqlite3_open
#undef sqlite3_open
#endif
#define sqlite3_open            sqlite3_export_relational_symbols->open

#ifdef sqlite3_open16
#undef sqlite3_open16
#endif
#define sqlite3_open16          sqlite3_export_relational_symbols->open16

#ifdef sqlite3_open_v2
#undef sqlite3_open_v2
#endif
#define sqlite3_open_v2         sqlite3_export_relational_symbols->open_v2
#endif // RELATIONAL_STORE_EXT_H