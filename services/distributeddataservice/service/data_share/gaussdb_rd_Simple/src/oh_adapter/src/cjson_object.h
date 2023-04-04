// /*
// * Copyright (c) 2023 Huawei Device Co., Ltd.
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// *
// *     http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// */

// #ifndef CJSON_OBJECT_H
// #define CJSON_OBJECT_H

// #include <string>
// #include "cJSON.h"
// #include "json_object.h"

// namespace DocumentDB {
// class CjsonObject : public JsonObject{
// public:
//     CjsonObject () = default;
//     virtual ~CjsonObject () = default;

//     int Parse(const std::string &str) override;
//     std::string Print() override;
//     CjsonObject* GetObjectItem(const std::string &field) override;
//     CjsonObject* GetArrayItem(const int index) override;
//     CjsonObject* GetNext();
//     CjsonObject* GetChild();
//     int DeleteItemFromObject(const std::string &field) override;
//     ResultValue* GetItemValue() override;
//     std::string GetItemFiled() override;
//     int Delete() override;

//     bool IsFieldExists(const JsonFieldPath &path) override;
//     int GetObjectByPath(const JsonFieldPath &path, ValueObject &obj) override;
// private:
//     cJSON *cjson_;
// };
// } // DocumentDB
// #endif // CJSON_OBJECT_H

