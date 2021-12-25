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
#ifndef AUTO_LAUNCH_CALLBACK_H
#define AUTO_LAUNCH_CALLBACK_H

#include <string>
#include "auto_launch_export.h"
#include "types_export.h"

#ifdef RELEASE_MODE_V2
class AutoLaunchCallback {
public:
    AutoLaunchCallback() {}
    ~AutoLaunchCallback() {}

    // Delete the copy and assign constructors
    AutoLaunchCallback(const AutoLaunchCallback &callback) = delete;
    AutoLaunchCallback &operator=(const AutoLaunchCallback &callback) = delete;
    AutoLaunchCallback(AutoLaunchCallback &&callback) = delete;
    AutoLaunchCallback &operator=(AutoLaunchCallback &&callback) = delete;

    void AutoLaunchNotifier(const std::string &userId, const std::string &appId, const std::string &storeId,
        DistributedDB::AutoLaunchStatus status);
    int GetStatus();
    void Clear();
#ifdef RELEASE_MODE_V3
    bool AutoLaunchRequestNotifier(const std::string &identifier, DistributedDB::AutoLaunchParam &param);
    void AddHashIdentity(const std::string &hashIdentity);
    void ClearHashIdentities();
    void SetAutoLaunchParam(DistributedDB::AutoLaunchParam &autoLaunchParam);
#endif

private:
    int realStatus_ = 0;
    std::vector<std::string> hashIdentities_;
#ifdef RELEASE_MODE_V3
    DistributedDB::AutoLaunchParam autoLaunchParam_;
#endif
};
#endif // end of RELEASE_MODE_V2
#endif // AUTO_LAUNCH_CALLBACK_H