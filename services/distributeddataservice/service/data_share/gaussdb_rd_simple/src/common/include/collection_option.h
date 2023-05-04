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

#ifndef COLLECTION_OPTION_H
#define COLLECTION_OPTION_H

#include <cstdint>
#include <string>

namespace DocumentDB {
class CollectionOption final {
public:
    static CollectionOption ReadOption(const std::string &optStr, int &errCode);

    uint32_t GetMaxDoc() const;
    std::string ToString() const;

    bool operator==(const CollectionOption &targetOption) const;
    bool operator!=(const CollectionOption &targetOption) const;

private:
    std::string option_ = "{}";
    uint32_t maxDoc_ = UINT32_MAX;
};
} // namespace DocumentDB
#endif // COLLECTION_OPTION_H