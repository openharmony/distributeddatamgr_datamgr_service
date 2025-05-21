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

#ifndef DISTRIBUTEDDATAMGR_EXTEND_HEAD_HANDLER_H
#define DISTRIBUTEDDATAMGR_EXTEND_HEAD_HANDLER_H
#include <string>

#include "iprocess_communicator.h"

namespace OHOS::DistributedData {
class RouteHeadHandler : public DistributedDB::ExtendHeaderHandle {
public:
    using ExtendInfo = DistributedDB::ExtendInfo;
    using DBStatus = DistributedDB::DBStatus;
    using UserInfo = DistributedDB::UserInfo;

    virtual bool ParseHeadDataLen(const uint8_t *data, uint32_t totalLen, uint32_t &headSize,
        const std::string &device) = 0;
    virtual bool ParseHeadDataUser(const uint8_t *data, uint32_t totalLen, const std::string &label,
        std::vector<UserInfo> &userInfos) = 0;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_EXTEND_HEAD_HANDLER_H
