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

#include "query_fuzzer.h"
#include "get_query_info.h"

using namespace DistributedDB;
using namespace std;

namespace {
    const std::string TEST_FIELD_NAME = "$.test";
}

namespace OHOS {
    void FuzzEqualTo(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().EqualTo(rawString, static_cast<int>(size));
    }

    void FuzzNotEqualTo(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().NotEqualTo(TEST_FIELD_NAME,rawString);
    }

    void FuzzGreaterThan(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().GreaterThan(rawString, static_cast<int>(U32_AT(data)));
    }

    void FuzzLessThan(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().LessThan(TEST_FIELD_NAME,rawString);
    }

    void FuzzGreaterThanOrEqualTo(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().GreaterThanOrEqualTo(rawString, static_cast<int>(size));
    }

    void FuzzLessThanOrEqualTo(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().LessThanOrEqualTo(TEST_FIELD_NAME,rawString);
    }

    void FuzzOrderBy(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().GreaterThanOrEqualTo(rawString, true);
        query = Query::Select().GreaterThanOrEqualTo(rawString, false);
    }

    void FuzzLimit(const uint8_t* data, size_t size)
    {
        Query query = Query::Select().Limit(static_cast<int>(size), static_cast<int>(U32_AT(data)));
    }

    void FuzzLike(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().Like(rawString, rawString);
    }

    void FuzzNotLike(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().NotLike(TEST_FIELD_NAME,rawString);
    }

    void FuzzIn(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        std::vector<std::string> values;
        //512 max size
        for (int i = 0; i < static_cast<int>(U32_AT(data) % 512); i++) {
            values.push_back(rawString);
        }
        Query query = Query::Select().In(TEST_FIELD_NAME, values);
    }

    void FuzzNotIn(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        std::vector<std::string> values;
        //512 max size
        for (int i = 0; i < static_cast<int>(size % 512); i++) {
            values.push_back(rawString);
        }
        Query query = Query::Select().NotIn(TEST_FIELD_NAME, values);
    }

    void FuzzIsNull(const uint8_t* data, size_t size)
    {
        std::string rawString(reinterpret_cast<const char *>(data), size);
        Query query = Query::Select().IsNull(rawString);
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    //u32 4 bytes
    if (size < 4) {
        return 0;
    }
    //Run your code on data
    OHOS::FuzzEqualTo(data,size);
    OHOS::FuzzNotEqualTo(data,size);
    OHOS::FuzzGreaterThan(data,size);
    OHOS::FuzzLessThan(data,size);
    OHOS::FuzzGreaterThanOrEqualTo(data,size);
    OHOS::FuzzLessThanOrEqualTo(data,size);
    OHOS::FuzzOrderBy(data,size);
    OHOS::FuzzLimit(data,size);
    OHOS::FuzzLike(data,size);
    OHOS::FuzzNotLike(data,size);
    OHOS::FuzzIn(data,size);
    OHOS::FuzzNotIn(data,size);
    OHOS::FuzzIsNull(data,size);
    return 0;
}

