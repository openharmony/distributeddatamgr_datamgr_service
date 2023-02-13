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

    bool isSingleApp;
    auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    if (!IsValidParams(uriInfo, PermissionType::WRITE_PERMISSION, isSingleApp, userId)) {
        ZLOGE("Params inValid! bundleName is %{public}s",
            DistributedData::Anonymous::Change(uriInfo.bundleName).c_str());
        return ERROR;
    }

    userId = ConvertUserId(userId, isSingleApp);
    int32_t ret = RdbAdaptor::Insert(uriInfo, valuesBucket, userId);
    if (ret == ERROR) {
        ZLOGE("Insert error %{public}s", DistributedData::Anonymous::Change(uri).c_str());
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

    bool isSingleApp;
    auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    if (!IsValidParams(uriInfo, PermissionType::WRITE_PERMISSION, isSingleApp, userId)) {
        ZLOGE("Params inValid! bundleName is %{public}s",
            DistributedData::Anonymous::Change(uriInfo.bundleName).c_str());
        return ERROR;
    }

    userId = ConvertUserId(userId, isSingleApp);
    int32_t ret = RdbAdaptor::Update(uriInfo, predicate, valuesBucket, userId);
    if (ret == ERROR) {
        ZLOGE("Update error %{public}s", DistributedData::Anonymous::Change(uri).c_str());
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

    bool isSingleApp;
    auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    if (!IsValidParams(uriInfo, PermissionType::WRITE_PERMISSION, isSingleApp, userId)) {
        ZLOGE("Params inValid! bundleName is %{public}s",
            DistributedData::Anonymous::Change(uriInfo.bundleName).c_str());
        return ERROR;
    }

    userId = ConvertUserId(userId, isSingleApp);
    int32_t ret = RdbAdaptor::Delete(uriInfo, predicate, userId);
    if (ret == ERROR) {
        ZLOGE("Delete error %{public}s", DistributedData::Anonymous::Change(uri).c_str());
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

    bool isSingleApp;
    auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    if (!IsValidParams(uriInfo, PermissionType::WRITE_PERMISSION, isSingleApp, userId)) {
        ZLOGE("Params inValid! bundleName is %{public}s",
            DistributedData::Anonymous::Change(uriInfo.bundleName).c_str());
        return nullptr;
    }

    userId = ConvertUserId(userId, isSingleApp);
    return RdbAdaptor::Query(uriInfo, predicates, columns, userId);
}

bool DataShareServiceImpl::IsValidParams(UriInfo &uriInfo,
    DataShareServiceImpl::PermissionType permissionType, bool &isSingleApp, int32_t userId)
{
    std::string permission;
    uint32_t tokenID = IPCSkeleton::GetCallingTokenID();
    AppExecFwk::BundleInfo bundleInfo;
    if (!PermissionProxy::GetBundleInfo(uriInfo.bundleName, tokenID, bundleInfo)) {
        ZLOGE("GetBundleInfo failed, BundleName is %{public}s, tokenId is %{public}u",
            DistributedData::Anonymous::Change(uriInfo.bundleName).c_str(), tokenId);
        return false;
    }
    switch (permissionType) {
        case PermissionType::READ_PERMISSION: {
            bool ret = PermissionProxy::QueryReadPermission(uriInfo.bundleName, tokenID, permission, bundleInfo);
            if (!ret) {
                ZLOGE("Query read permission failed!");
                return false;
            }
            break;
        }
        case PermissionType::WRITE_PERMISSION: {
            bool ret = PermissionProxy::QueryWritePermission(uriInfo.bundleName, tokenID, permission, bundleInfo);
            if (!ret) {
                ZLOGE("Query write permission failed!");
                return false;
            }
            break;
        }
    }

    ProfileInfo profileInfo;
    if (!dataShareProfileInfo_.LoadProfileInfoFromExtension(bundleInfo, profileInfo, isSingleApp)) {
        ZLOGE("LoadProfileInfoFromExtension failed!");
        return false;
    }
    if (!PermissionProxy::ConvertTableNameByCrossUserMode(profileInfo, userId, isSingleApp, uriInfo)) {
        ZLOGE("ConvertTableNameByCrossUserMode failed!");
        return false;
    }
    return true;
}

int32_t DataShareServiceImpl::ConvertUserId(int32_t userId, bool isSingleApp)
{
    if (isSingleApp) {
        ZLOGD("This hap is single app");
        return 0;
    }
    return userId;
}
} // namespace OHOS::DataShare
