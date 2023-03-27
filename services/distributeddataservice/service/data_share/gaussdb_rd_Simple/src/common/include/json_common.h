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

#ifndef JSON_COMMON_H
#define JSON_COMMON_H

#include <cstdint>
#include <vector>
#include "cjson_object.h"

namespace DocumentDB {
class JsonCommon
{
public:
    JsonCommon() = default;
    ~JsonCommon();
    
    ResultValue* GetValue(CjsonObject *root, std::vector<std::string> path);
    static int GetIdValue(CjsonObject *root, std::vector<std::string> &id);
    static bool CheckIsJson(const std::string &data);
    static int GetJsonDeep(const std::string &data);
};

} // DocumentDB
#endif // JSON_COMMON_H