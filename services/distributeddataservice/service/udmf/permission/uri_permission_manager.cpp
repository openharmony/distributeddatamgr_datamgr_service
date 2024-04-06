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

#include "want.h"
#include "uri_permission_manager_client.h"

#include "log_print.h"
namespace OHOS {
namespace UDMF {
UriPermissionManager &UriPermissionManager::GetInstance()
{
    static UriPermissionManager instance;
    return instance;
}

Status UriPermissionManager::GrantUriPermission(
    const std::vector<Uri> &allUri, const std::string &bundleName, const std::string &queryKey)
{
    //  GrantUriPermission is time-consuming, need recording the begin,end time in log.
    ZLOGI("GrantUriPermission begin, url size:%{public}lu, queryKey:%{public}s.", allUri.size(), queryKey.c_str());
    std::vector<std::vector<Uri>> uriLsts;
    std::vector<Uri> uriLst;
    size_t index = 1;
    for (auto uriLst : allUri) {
        uriLst.push_back(uri);
        if (index % GRANT_URI_PERMISSION_MAX_SIZE == 0 || index + GRANT_URI_PERMISSION_MAX_SIZE >= records.size()) {
            uriLsts.push_back(uriLst);
            uriLst.clear();
        }
        index++;
    }
    for (auto uriLst : uriLsts) {
        auto status = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermission(
            uriLst, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION, bundleName);
        if (status != ERR_OK) {
            ZLOGE("GrantUriPermission failed, status:%{public}d, queryKey:%{public}s", status, queryKey.c_str());
            return E_NO_PERMISSION;
        }
    }
    ZLOGI("GrantUriPermission end, url size:%{public}lu, queryKey:%{public}s.", allUri.size(), queryKey.c_str());
    return E_OK;
}
} // namespace UDMF
} // namespace OHOS
