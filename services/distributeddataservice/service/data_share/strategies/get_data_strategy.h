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

#ifndef DATASHARESERVICE_GET_DATA_STRAGETY_H
#define DATASHARESERVICE_GET_DATA_STRAGETY_H

#include <shared_mutex>

#include "data_proxy_observer.h"
#include "datashare_template.h"
#include "published_data.h"
#include "seq_strategy.h"

namespace OHOS::DataShare {
class GetDataStrategy final {
public:
    Data Execute(std::shared_ptr<Context> context, int &errorCode);

private:
    SeqStrategy &GetStrategy();
    std::mutex mutex_;
    SeqStrategy strategies_;
    bool CheckPermission(std::shared_ptr<Context> context, const std::string &key);
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_GET_DATA_STRAGETY_H
