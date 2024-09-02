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
#define LOG_TAG "RuntimeStoreAccountObserver"

#include "store_account_observer.h"
#include "runtime_store.h"
#include "log_print.h"
#include "directory/directory_manager.h"
#include "account/account_delegate.h"
#include "bootstrap.h"

namespace OHOS {
namespace UDMF {
using namespace DistributedKv;
using namespace DistributedData;
void RuntimeStoreAccountObserver::OnAccountChanged(const DistributedKv::AccountEventInfo &eventInfo)
{
     ZLOGI("account event begin. userId is %{public}s, status is %{public}d.", eventInfo.userId.c_str(), eventInfo.status);
     if (eventInfo.status == DistributedKv::AccountStatus::DEVICE_ACCOUNT_DELETE) {
          DistributedData::StoreMetaData metaData;

          metaData.appType = "harmony";
          // metaData.storeId = storeId_;
          metaData.isAutoSync = false;
          metaData.isBackup = false;
          metaData.isEncrypt = false;
          metaData.bundleName = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
          metaData.appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
          metaData.user = eventInfo.userId;
          metaData.securityLevel = DistributedKv::SecurityLevel::S1;
          metaData.area = DistributedKv::Area::EL1;
          metaData.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
          metaData.dataType = DistributedKv::DataType::TYPE_DYNAMICAL;
          metaData.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(metaData);

          std::string userPath = metaData.dataDir.append("/").append(eventInfo.userId);
          ZLOGI("userPath is %{public}s", userPath.c_str());
          DistributedData::DirectoryManager::GetInstance().DeleteDirectory(userPath.c_str());
     }
}

} // namespace UDMF
} // namespace OHOS