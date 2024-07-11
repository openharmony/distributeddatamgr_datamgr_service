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
#define LOG_TAG "UriPermissionManager"

#include "uri_permission_manager.h"

#include "log_print.h"
#include "preprocess_utils.h"
#include "uri_permission_manager_client.h"
#include "want.h"

namespace OHOS {
namespace UDMF {
constexpr const std::uint32_t GRANT_URI_PERMISSION_MAX_SIZE = 500;
UriPermissionManager &UriPermissionManager::GetInstance()
{
    static UriPermissionManager instance;
    return instance;
}

Status UriPermissionManager::GrantUriPermission(
    const std::vector<Uri> &allUri, uint32_t tokenId, const std::string &queryKey)
{
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(tokenId, bundleName)) {
        ZLOGE("Get BundleName fail, key:%{public}s, tokenId:%{public}u.", queryKey.c_str(), tokenId);
        return E_ERROR;
    }
    int32_t instIndex = -1;
    if (!PreProcessUtils::GetInstIndex(tokenId, instIndex)) {
        ZLOGE("Get InstIndex fail, key:%{public}s, tokenId:%{public}u.", queryKey.c_str(), tokenId);
        return E_ERROR;
    }

    //  GrantUriPermission is time-consuming, need recording the begin,end time in log.
    ZLOGI("GrantUriPermission begin, url size:%{public}zu, queryKey:%{public}s, instIndex:%{public}d.",
        allUri.size(), queryKey.c_str(), instIndex);
    for (size_t index = 0; index < allUri.size(); index += GRANT_URI_PERMISSION_MAX_SIZE) {
        std::vector<Uri> uriLst(
            allUri.begin() + index, allUri.begin() + std::min(index + GRANT_URI_PERMISSION_MAX_SIZE, allUri.size()));
        auto status = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermissionPrivileged(
            uriLst, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION, bundleName, instIndex);
        if (status != ERR_OK) {
            ZLOGE("GrantUriPermission failed, status:%{public}d, queryKey:%{public}s, instIndex:%{public}d.",
                status, queryKey.c_str(), instIndex);
            return E_NO_PERMISSION;
        }
        auto time = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
        std::for_each(uriLst.begin(), uriLst.end(), [&](const Uri &uri) {
            auto times = std::make_pair(uri.ToString(), tokenId);
            uriTimeout_.Insert(times, time);
        });
    }
    ZLOGI("GrantUriPermission end, url size:%{public}zu, queryKey:%{public}s.", allUri.size(), queryKey.c_str());

    std::unique_lock<std::mutex> lock(taskMutex_);
    if (taskId_ == ExecutorPool::INVALID_TASK_ID && executorPool_ != nullptr) {
        taskId_ = executorPool_->Schedule(
            std::chrono::minutes(INTERVAL), std::bind(&UriPermissionManager::RevokeUriPermission, this));
    }
    return E_OK;
}

void UriPermissionManager::RevokeUriPermission()
{
    auto current = std::chrono::steady_clock::now();
    uriTimeout_.EraseIf([&](auto &key, Time &time) {
        if (time > current) {
            return false;
        }
        Uri uri(key.first);
        uint32_t tokenId = key.second;
        std::string bundleName;
        if (!PreProcessUtils::GetHapBundleNameByToken(tokenId, bundleName)) {
            ZLOGE("Get BundleName fail, tokenId:%{public}u.", tokenId);
            return true;
        }
        int32_t instIndex = -1;
        if (!PreProcessUtils::GetInstIndex(tokenId, instIndex)) {
            ZLOGE("Get InstIndex fail, tokenId:%{public}u.", tokenId);
            return true;
        }
        int status = AAFwk::UriPermissionManagerClient::GetInstance().RevokeUriPermissionManually(
            uri, bundleName, instIndex);
        if (status != E_OK) {
            ZLOGE("RevokeUriPermission error, permissionCode:%{public}d, bundleName:%{public}s, instIndex:%{public}d",
                status, bundleName.c_str(), instIndex);
        }
        return true;
    });

    std::unique_lock<std::mutex> lock(taskMutex_);
    if (!uriTimeout_.Empty() && executorPool_ != nullptr) {
        ZLOGD("RevokeUriPermission, uriTimeout size:%{public}zu", uriTimeout_.Size());
        taskId_ = executorPool_->Schedule(
            std::chrono::minutes(INTERVAL), std::bind(&UriPermissionManager::RevokeUriPermission, this));
    } else {
        taskId_ = ExecutorPool::INVALID_TASK_ID;
    }
}

void UriPermissionManager::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
    executorPool_ = executors;
}
} // namespace UDMF
} // namespace OHOS
