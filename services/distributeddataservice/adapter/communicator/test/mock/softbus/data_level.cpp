/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "data_level.h"

static constexpr const char *REMOTE_NETWORK_ID = "remote_network_id";

SoftBusErrNo errCode_;
IDataLevelCb *callback_ = nullptr;

void ConfigReturnCode(SoftBusErrNo errorCode)
{
    errCode_ = errorCode;
}

int32_t RegDataLevelChangeCb(const char *pkgName, IDataLevelCb *callback)
{
    (void)pkgName;
    callback_ = callback;
    return errCode_;
}

int32_t UnregDataLevelChangeCb(const char *pkgName)
{
    (void)pkgName;
    callback_ = nullptr;
    return SoftBusErrNo::SOFTBUS_OK;
}

int32_t SetDataLevel(const DataLevel *dataLevel)
{
    if (errCode_ == SoftBusErrNo::SOFTBUS_OK && callback_ != nullptr) {
        callback_->onDataLevelChanged(REMOTE_NETWORK_ID, *dataLevel);
    }
    return errCode_;
}