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

#define LOG_TAG "KvStoreUserManager"

#include "kvstore_user_manager.h"
#include "account_delegate.h"
#include "checker/checker_manager.h"
#include "constant.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "permission_validator.h"

#define DEFAUL_RETRACT "    "

namespace OHOS {
namespace DistributedKv {
using namespace DistributedData;
KvStoreUserManager::KvStoreUserManager(const std::string &userId)
    : appMutex_(),
      appMap_(),
      userId_(userId)
{
    ZLOGI("begin.");
}

KvStoreUserManager::~KvStoreUserManager()
{
    ZLOGI("begin.");
    appMap_.clear();
}

Status KvStoreUserManager::CloseKvStore(const std::string &appId, const std::string &storeId)
{
    ZLOGI("begin.");
    std::lock_guard<decltype(appMutex_)> lg(appMutex_);
    auto it = appMap_.find(appId);
    if (it != appMap_.end()) {
        return (it->second).CloseKvStore(storeId);
    }

    ZLOGE("store not open.");
    return Status::STORE_NOT_OPEN;
}

Status KvStoreUserManager::CloseAllKvStore(const std::string &appId)
{
    ZLOGI("begin.");
    std::lock_guard<decltype(appMutex_)> lg(appMutex_);
    auto it = appMap_.find(appId);
    if (it != appMap_.end()) {
        return (it->second).CloseAllKvStore();
    }

    ZLOGE("store not open.");
    return Status::STORE_NOT_OPEN;
}

void KvStoreUserManager::CloseAllKvStore()
{
    ZLOGI("begin.");
    std::lock_guard<decltype(appMutex_)> lg(appMutex_);
    for (auto &it : appMap_) {
        (it.second).CloseAllKvStore();
    }
}

Status KvStoreUserManager::DeleteKvStore(
    const std::string &bundleName, pid_t uid, uint32_t token, const std::string &storeId)
{
    ZLOGI("begin.");
    std::lock_guard<decltype(appMutex_)> lg(appMutex_);
    auto it = appMap_.find(bundleName);
    if (it != appMap_.end()) {
        auto status = (it->second).DeleteKvStore(storeId);
        if ((it->second).GetTotalKvStoreNum() == 0) {
            ZLOGI("There is not kvstore, so remove the  app manager.");
            appMap_.erase(it);
        }
        return status;
    }
    KvStoreAppManager kvStoreAppManager(bundleName, uid, token);
    return kvStoreAppManager.DeleteKvStore(storeId);
}

void KvStoreUserManager::DeleteAllKvStore()
{
    ZLOGI("begin.");
    std::lock_guard<decltype(appMutex_)> lg(appMutex_);
    for (auto &it : appMap_) {
        (it.second).DeleteAllKvStore();
    }
    appMap_.clear();
}

void KvStoreUserManager::Dump(int fd) const
{
    dprintf(fd, DEFAUL_RETRACT"--------------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"UserID        : %s\n", userId_.c_str());
    dprintf(fd, DEFAUL_RETRACT"App count     : %u\n", static_cast<uint32_t>(appMap_.size()));
    for (const auto &pair : appMap_) {
        pair.second.Dump(fd);
    }
}

void KvStoreUserManager::DumpUserInfo(int fd) const
{
    dprintf(fd, DEFAUL_RETRACT"--------------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"UserID        : %s\n", userId_.c_str());
    dprintf(fd, DEFAUL_RETRACT"App count     : %u\n", static_cast<uint32_t>(appMap_.size()));
    for (const auto &pair : appMap_) {
        pair.second.DumpUserInfo(fd);
    }
}

void KvStoreUserManager::DumpAppInfo(int fd, const std::string &appId) const
{
    dprintf(fd, DEFAUL_RETRACT"--------------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"UserID        : %s\n", userId_.c_str());
    if (appId != "") {
        auto it = appMap_.find(appId);
        if (it != appMap_.end()) {
            it->second.DumpAppInfo(fd);
            return;
        }
    }

    for (const auto &pair : appMap_) {
        pair.second.DumpAppInfo(fd);
    }
}

void KvStoreUserManager::DumpStoreInfo(int fd, const std::string &storeId) const
{
    dprintf(fd, DEFAUL_RETRACT"--------------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"UserID        : %s\n", userId_.c_str());
    for (const auto &pair : appMap_) {
        pair.second.DumpStoreInfo(fd, storeId);
    }
}

bool KvStoreUserManager::IsStoreOpened(const std::string &appId, const std::string &storeId)
{
    std::lock_guard<decltype(appMutex_)> lg(appMutex_);
    auto it = appMap_.find(appId);
    return it != appMap_.end() && it->second.IsStoreOpened(storeId);
}

void KvStoreUserManager::SetCompatibleIdentify(const std::string &deviceId) const
{
    std::lock_guard<decltype(appMutex_)> lg(appMutex_);
    for (const auto &item : appMap_) {
        item.second.SetCompatibleIdentify(deviceId);
    }
}
}  // namespace DistributedKv
}  // namespace OHOS
