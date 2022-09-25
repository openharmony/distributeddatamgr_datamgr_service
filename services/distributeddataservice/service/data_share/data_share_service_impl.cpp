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

#define LOG_TAG "DataShareServiceImpl"

#include "data_share_service_impl.h"

#include "accesstoken_kit.h"
#include "bundle_constants.h"
#include "dataobs_mgr_client.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "permission_proxy.h"
#include "rdb_adaptor.h"
#include "uri.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
bool DataShareServiceImpl::CheckCallingPermission(const std::string &permission)
{
    if (!permission.empty()
        && Security::AccessToken::AccessTokenKit::VerifyAccessToken(IPCSkeleton::GetCallingTokenID(), permission)
            != AppExecFwk::Constants::PERMISSION_GRANTED) {
        ZLOGE("permission not granted.");
        return false;
    }
    return true;
}

int32_t DataShareServiceImpl::Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket)
{
    ZLOGD("enter");
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string tableName;
    bool ret = URIUtils::GetInfoFromURI(uri, bundleName, moduleName, storeName, tableName);
    if (!ret) {
        ZLOGE("uri error %{public}s", uri.c_str());
        return -1;
    }
    std::string permission;
    ret = PermissionProxy::QueryWritePermission(bundleName, moduleName, storeName, permission);
    if (!ret) {
        ZLOGE("query permission failed");
        return -1;
    }
    ret = CheckCallingPermission(permission);
    if (!ret) {
        ZLOGE("no permission %{public}s", permission.c_str());
        return -1;
    }

    ret = RdbAdaptor::Insert(bundleName, moduleName, storeName, tableName, valuesBucket);
    if (!ret) {
        ZLOGE("Insert error %{public}s", uri.c_str());
        return -1;
    }
    NotifyChange(uri);
    return ret;
}

bool DataShareServiceImpl::NotifyChange(const std::string &uri)
{
    ZLOGD("Start");
    ZLOGE("NotifyChange Uri = %{public}s", uri.c_str());
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        ZLOGE("obsMgrClient is nullptr");
        return false;
    }

    ErrCode ret = obsMgrClient->NotifyChange(Uri(uri));
    if (ret != ERR_OK) {
        ZLOGE("obsMgrClient->NotifyChange error return %{public}d", ret);
        return false;
    }
    return true;
}

int32_t DataShareServiceImpl::Update(
    const std::string &uri, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket)
{
    ZLOGD("enter");
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string tableName;
    bool ret = URIUtils::GetInfoFromURI(uri, bundleName, moduleName, storeName, tableName);
    if (!ret) {
        ZLOGE("uri error %{public}s", uri.c_str());
        return -1;
    }
    std::string permission;
    ret = PermissionProxy::QueryWritePermission(bundleName, moduleName, storeName, permission);
    if (!ret) {
        ZLOGE("query permission failed");
        return -1;
    }
    ret = CheckCallingPermission(permission);
    if (!ret) {
        ZLOGE("no permission %{public}s", permission.c_str());
        return -1;
    }
    ret = RdbAdaptor::Update(bundleName, moduleName, storeName, tableName, predicate, valuesBucket);
    if (!ret) {
        ZLOGE("Update error %{public}s", uri.c_str());
        return -1;
    }
    NotifyChange(uri);
    return ret;
}

int32_t DataShareServiceImpl::Delete(const std::string &uri, const DataSharePredicates &predicate)
{
    ZLOGD("enter");
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string tableName;
    bool ret = URIUtils::GetInfoFromURI(uri, bundleName, moduleName, storeName, tableName);
    if (!ret) {
        ZLOGE("uri error %{public}s", uri.c_str());
        return -1;
    }
    std::string permission;
    ret = PermissionProxy::QueryWritePermission(bundleName, moduleName, storeName, permission);
    if (!ret) {
        ZLOGE("query permission failed");
        return -1;
    }
    ret = CheckCallingPermission(permission);
    if (!ret) {
        ZLOGE("no permission %{public}s", permission.c_str());
        return -1;
    }

    ret = RdbAdaptor::Delete(bundleName, moduleName, storeName, tableName, predicate);
    if (!ret) {
        ZLOGE("Delete error %{public}s", uri.c_str());
        return -1;
    }
    NotifyChange(uri);
    return ret;
}

std::shared_ptr<DataShareResultSet> DataShareServiceImpl::Query(
    const std::string &uri, const DataSharePredicates &predicates, const std::vector<std::string> &columns)
{
    ZLOGD("enter");
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string tableName;
    bool ret = URIUtils::GetInfoFromURI(uri, bundleName, moduleName, storeName, tableName);
    if (!ret) {
        ZLOGE("uri error %{public}s", uri.c_str());
        return nullptr;
    }
    std::string permission;
    ret = PermissionProxy::QueryReadPermission(bundleName, moduleName, storeName, permission);
    if (!ret) {
        ZLOGE("query permission failed");
        return nullptr;
    }
    ret = CheckCallingPermission(permission);
    if (!ret) {
        ZLOGE("no permission %{public}s", permission.c_str());
        return nullptr;
    }

    return RdbAdaptor::Query(bundleName, moduleName, storeName, tableName, predicates, columns);
}
} // namespace OHOS::DataShare
