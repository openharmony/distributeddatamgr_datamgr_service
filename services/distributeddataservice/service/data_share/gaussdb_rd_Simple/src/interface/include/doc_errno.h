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

#ifndef DOC_ERRNO_H
#define DOC_ERRNO_H


namespace DocumentDB {
constexpr int E_OK = 0;
constexpr int E_BASE = 1000;
constexpr int E_ERROR = E_BASE + 1;
constexpr int E_INVALID_ARGS = E_BASE + 2;
constexpr int E_UNFINISHED = E_BASE + 7;
constexpr int E_OUT_OF_MEMORY = E_BASE + 8;
constexpr int E_SECUREC_ERROR = E_BASE + 9;
constexpr int E_SYSTEM_API_FAIL = E_BASE + 10;
} // DocumentDB
#endif // DOC_ERRNO_H