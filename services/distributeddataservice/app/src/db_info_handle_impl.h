/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTEDDATAMGR_DB_INFO_HANDLE_IMPL_H
#define DISTRIBUTEDDATAMGR_DB_INFO_HANDLE_IMPL_H

#include "db_info_handle.h"
namespace OHOS::DistributedKv {
using namespace DistributedDB;
class DBInfoHandleImpl : public DBInfoHandle {
public:
    // return true if you can notify with RuntimeConfig::NotifyDBInfo
    bool IsSupport() override;
    // check is need auto sync when remote db is open
    // return true if data has changed
    bool IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
        const DeviceInfos &devInfo) override;
};
}
#endif //DISTRIBUTEDDATAMGR_DB_INFO_HANDLE_IMPL_H
