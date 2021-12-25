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
#include "auto_launch_callback.h"

#include <algorithm>
#include "distributeddb_log_print.h"

using namespace DistributedDB;
using namespace std;

#ifdef RELEASE_MODE_V2
void AutoLaunchCallback::AutoLaunchNotifier(const std::string &userId, const std::string &appId,
    const std::string &storeId, AutoLaunchStatus status)
{
    MST_LOG("The db: %s,%s,%s is set to sync by closed status and comes some data changes now.", userId.c_str(),
        appId.c_str(), storeId.c_str());
    this->realStatus_ = status;
}

int AutoLaunchCallback::GetStatus()
{
    return realStatus_;
}

void AutoLaunchCallback::Clear()
{
    this->realStatus_ = 0;
}

#ifdef RELEASE_MODE_V3
bool AutoLaunchCallback::AutoLaunchRequestNotifier(const std::string &identifier, AutoLaunchParam &param)
{
    auto iter = find(hashIdentities_.begin(), hashIdentities_.end(), identifier);
    if (iter != hashIdentities_.end()) {
        param.userId = autoLaunchParam_.userId;
        param.appId = autoLaunchParam_.appId;
        param.storeId = autoLaunchParam_.storeId;
        param.option = autoLaunchParam_.option;
        param.notifier = autoLaunchParam_.notifier;
        return true;
    }
    return false;
}

void AutoLaunchCallback::AddHashIdentity(const string &hashIdentity)
{
    hashIdentities_.push_back(hashIdentity);
}

void AutoLaunchCallback::ClearHashIdentities()
{
    hashIdentities_.clear();
}

void AutoLaunchCallback::SetAutoLaunchParam(AutoLaunchParam &autoLaunchParam)
{
    autoLaunchParam_ = autoLaunchParam;
}
#endif // end of RELEASE_MODE_V3
#endif // end of RELEASE_MODE_V2