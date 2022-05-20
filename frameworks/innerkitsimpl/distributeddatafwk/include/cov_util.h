/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_INNERKITSIMPL_NATIVE_PREFERENCES_INCLUDE_COV_UTIL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_INNERKITSIMPL_NATIVE_PREFERENCES_INCLUDE_COV_UTIL_H

#include <string>
#include <variant>
#include <set>
#include "data_query.h"

namespace OHOS {
namespace DistributedKv {
class CovUtil final {
public:
    template<typename _VTp, typename _TTp, typename _First>
    static auto FillField(const std::string &field, const _VTp &data, _TTp &target)
    {
        return target();
    }

    template<typename _VTp, typename _TTp, typename _First, typename _Second, typename ..._Rest>
    static auto FillField(const std::string &field, const _VTp &data, _TTp &target)
    {
        if ((sizeof ...(_Rest) + 1) == data.index()) {
            return target(field, std::get<(sizeof ...(_Rest) + 1)>(data));
        }
        return FillField<_VTp, _TTp, _Second, _Rest...>(field, data, target);
    }

    template<typename _TTp, typename ..._Types>
    static auto FillField(const std::string &field, const std::variant<_Types...> &data, _TTp &target)
    {
        return FillField<decltype(data), _TTp, _Types...>(field, data, target);
    }
};

class Equal  {
public:
    Equal(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        dataQuery_->EqualTo(field, value);
        return 0;
    }

    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};

class NotEqual  {
public:
    NotEqual(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        dataQuery_->NotEqualTo(field, value);
        return 0;
    }
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};

class Greater  {
public:
    Greater(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        dataQuery_->GreaterThan(field, value);
        return 0;
    }
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};

class Less  {
public:
    Less(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        dataQuery_->LessThan(field, value);
        return 0;
    }
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};

class GreaterOrEqual  {
public:
    GreaterOrEqual(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        dataQuery_->GreaterThanOrEqualTo(field, value);
        return 0;
    }
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};

class LessOrEqual  {
public:
    LessOrEqual(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        dataQuery_->LessThanOrEqualTo(field, value);
        return 0;
    }
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};

class In  {
public:
    In(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        return 0;
    }
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        dataQuery_->In(field, value);
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};

class NotIn  {
public:
    NotIn(OHOS::DistributedKv::DataQuery *dataQuery) : dataQuery_(dataQuery) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        return 0;
    }
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        dataQuery_->In(field, value);
        return 0;
    }

    int operator()()
    {
        return 0;
    }
private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
};
}
}

#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_INNERKITSIMPL_NATIVE_PREFERENCES_INCLUDE_COV_UTIL_H
