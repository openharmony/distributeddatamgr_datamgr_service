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
#include "account/account_delegate.h"
#include "bundle_constants.h"
#include "dataobs_mgr_client.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "permission_proxy.h"
#include "rdb_adaptor.h"
#include "uri.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
using FeatureSystem = DistributedData::FeatureSystem;
__attribute__((used)) DataShareServiceImpl::Factory DataShareServiceImpl::factory_;
DataShareServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator("data_share",
        []() { return std::make_shared<DataShareServiceImpl>(); });
}

DataShareServiceImpl::Factory::~Factory()
{
}

int32_t DataShareServiceImpl::Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket)
{
    ZLOGD("Insert enter.");
    UriInfo uriInfo;
    if (!URIUtils::GetInfoFromURI(uri, uriInfo)) {
        ZLOGE("GetInfoFromURI failed!");
        return ERROR;
    }

    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    AppExecFwk::BundleInfo bundleInfo;
    if (!PermissionProxy::GetBundleInfo(uriInfo.bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfo failed, BundleName is %{public}s, tokenId is %{public}u",
            uriInfo.bundleName.c_str(), tokenId);
        return ERROR;
    }

    if (!VerifyPermission(tokenId, PermissionType::WRITE_PERMISSION, bundleInfo)) {
        ZLOGE("Verify permission failed, bundleName is %{public}s", uriInfo.bundleName.c_str());
        return ERROR;
    }

    uriInfo.SetTableName(GetRealityTableName(tokenId, bundleInfo, uriInfo));
    auto userId = GetUserId(tokenId, bundleInfo.singleton);
    int32_t ret = RdbAdaptor::Insert(uriInfo, valuesBucket, userId);
    if (ret == ERROR) {
        ZLOGE("Insert error, uri is %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    NotifyChange(uri);
    return ret;
}

bool DataShareServiceImpl::NotifyChange(const std::string &uri)
{
    ZLOGD("Start");
    ZLOGE("NotifyChange Uri = %{public}s", DistributedData::Anonymous::Change(uri).c_str());
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

int32_t DataShareServiceImpl::Update(const std::string &uri, const DataSharePredicates &predicate,
    const DataShareValuesBucket &valuesBucket)
{
    ZLOGD("Update enter.");
    UriInfo uriInfo;
    if (!URIUtils::GetInfoFromURI(uri, uriInfo)) {
        ZLOGE("GetInfoFromURI failed!");
        return ERROR;
    }
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    AppExecFwk::BundleInfo bundleInfo;
    if (!PermissionProxy::GetBundleInfo(uriInfo.bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfo failed, BundleName is %{public}s, tokenId is %{public}u",
            uriInfo.bundleName.c_str(), tokenId);
        return ERROR;
    }

    if (!VerifyPermission(tokenId, PermissionType::WRITE_PERMISSION, bundleInfo)) {
        ZLOGE("Verify permission failed, bundleName is %{public}s", uriInfo.bundleName.c_str());
        return ERROR;
    }

    uriInfo.SetTableName(GetRealityTableName(tokenId, bundleInfo, uriInfo));
    auto userId = GetUserId(tokenId, bundleInfo.singleton);
    int32_t ret = RdbAdaptor::Update(uriInfo, predicate, valuesBucket, userId);
    if (ret == ERROR) {
        ZLOGE("Update error, uri is %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    NotifyChange(uri);
    return ret;
}

int32_t DataShareServiceImpl::Delete(const std::string &uri, const DataSharePredicates &predicate)
{
    ZLOGD("Delete enter.");
    UriInfo uriInfo;
    if (!URIUtils::GetInfoFromURI(uri, uriInfo)) {
        ZLOGE("GetInfoFromURI failed!");
        return ERROR;
    }

    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    AppExecFwk::BundleInfo bundleInfo;
    if (!PermissionProxy::GetBundleInfo(uriInfo.bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfo failed, BundleName is %{public}s, tokenId is %{public}u",
            uriInfo.bundleName.c_str(), tokenId);
        return ERROR;
    }

    if (!VerifyPermission(tokenId, PermissionType::WRITE_PERMISSION, bundleInfo)) {
        ZLOGE("Verify permission failed, bundleName is %{public}s", uriInfo.bundleName.c_str());
        return ERROR;
    }
    uriInfo.SetTableName(GetRealityTableName(tokenId, bundleInfo, uriInfo));
    auto userId = GetUserId(tokenId, bundleInfo.singleton);
    int32_t ret = RdbAdaptor::Delete(uriInfo, predicate, userId);
    if (ret == ERROR) {
        ZLOGE("Delete error, uri is %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    NotifyChange(uri);
    return ret;
}

std::shared_ptr<DataShareResultSet> DataShareServiceImpl::Query(const std::string &uri,
    const DataSharePredicates &predicates, const std::vector<std::string> &columns)
{
    ZLOGD("Query enter.");
    UriInfo uriInfo;
    if (!URIUtils::GetInfoFromURI(uri, uriInfo)) {
        ZLOGE("GetInfoFromURI failed!");
        return nullptr;
    }

    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    AppExecFwk::BundleInfo bundleInfo;
    if (!PermissionProxy::GetBundleInfo(uriInfo.bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfo failed, BundleName is %{public}s, tokenId is %{public}u",
            uriInfo.bundleName.c_str(), tokenId);
        return nullptr;
    }
    
    if (!VerifyPermission(tokenId, PermissionType::READ_PERMISSION, bundleInfo)) {
        ZLOGE("Verify permission failed, bundleName is %{public}s", uriInfo.bundleName.c_str());
        return nullptr;
    }
    uriInfo.SetTableName(GetRealityTableName(tokenId, bundleInfo, uriInfo));
    auto userId = GetUserId(tokenId, bundleInfo.singleton);
    return RdbAdaptor::Query(uriInfo, predicates, columns, userId);
}

std::string DataShareServiceImpl::GetRealityTableName(uint32_t tokenId, const AppExecFwk::BundleInfo &bundleInfo, const UriInfo &uriInfo)
{
    auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    bool isSingleApp;
    ProfileInfo profileInfo;
    if (!dataShareProfileInfo_.GetProfileInfoFromExtension(bundleInfo, profileInfo, isSingleApp)) {
        ZLOGE("LoadProfileInfoFromExtension failed, BundleName is %{public}s, tokenId is %{public}u",
            uriInfo.bundleName.c_str(), tokenId);
        return uriInfo.tableName;
    }
    return PermissionProxy::GetTableNameByCrossUserMode(profileInfo, userId, isSingleApp, uriInfo);
}


bool DataShareServiceImpl::VerifyPermission(uint32_t tokenID,
    DataShareServiceImpl::PermissionType permissionType, const AppExecFwk::BundleInfo &bundleInfo)
{
    std::string permission;
    switch (permissionType) {
        case PermissionType::READ_PERMISSION: {
            bool ret = PermissionProxy::QueryReadPermission(tokenID, permission, bundleInfo);
            if (!ret) {
                ZLOGE("Query read permission failed!");
                return false;
            }
            break;
        }
        case PermissionType::WRITE_PERMISSION: {
            bool ret = PermissionProxy::QueryWritePermission(tokenID, permission, bundleInfo);
            if (!ret) {
                ZLOGE("Query write permission failed!");
                return false;
            }
            break;
        }
    }
    return true;
}

int32_t DataShareServiceImpl::GetUserId(uint32_t tokenId, bool isSingleApp)
{
    if (isSingleApp) {
        ZLOGD("This hap is single app");
        return 0;
    }
    return DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
}
} // namespace OHOS::DataShare
