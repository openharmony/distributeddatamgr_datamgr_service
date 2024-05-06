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

#ifndef DATASHARESERVICE_APP_CONNECT_MANAGER_H
#define DATASHARESERVICE_APP_CONNECT_MANAGER_H

#include <memory>
#include <string>

#include "block_data.h"
#include "concurrent_map.h"

namespace OHOS::DataShare {
class AppConnectManager {
public:
    static bool Wait(const std::string &bundleName, int maxWaitTime, std::function<bool()> connect,
        std::function<void()> disconnect);
    static void Notify(const std::string &bundleName);

private:
    static ConcurrentMap<std::string, BlockData<bool> *> blockCache_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_APP_CONNECT_MANAGER_H
