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

#ifndef DISTRIBUTEDDATAMGR_DFX_TYPES_H
#define DISTRIBUTEDDATAMGR_DFX_TYPES_H

#include <map>
#include <memory>
#include <string>

namespace OHOS {
namespace DistributedDataDfx {
struct ModuleName {
    static constexpr const char *DEVICE = "DEVICE";
    static constexpr const char *USER = "USER";
    static constexpr const char *RDB_STORE = "RDB_STORE";
    static constexpr const char *KV_STORE = "KV_STORE";
    static constexpr const char *CLOUD_SERVER = "CLOUD_SERVER";
    static constexpr const char *CLOUD_SYNC_CALLBACK = "CLOUD_SYNC_CALLBACK";
};

enum class Fault {
    // Cloud Sync Fault
    CSF_CLOUD_INFO                          = 70,
    CSF_SUBSCRIBE                           = 71,
    CSF_UNSUBSCRIBE                         = 72,
    CSF_APP_SCHEMA                          = 73,
    CSF_CONNECT_CLOUD_ASSET_LOADER          = 74,
    CSF_CONNECT_CLOUD_DB                    = 75,
    CSF_BATCH_INSERT                        = 76,
    CSF_BATCH_UPDATE                        = 77,
    CSF_BATCH_DELETE                        = 78,
    CSF_BATCH_QUERY                         = 79,
    CSF_LOCK                                = 80,
    CSF_SHARE                               = 81,
    CSF_DOWNLOAD_ASSETS                     = 82,
    CSF_GS_CREATE_DISTRIBUTED_TABLE         = 83,
    CSF_GS_CLOUD_SYNC                       = 84,
    CSF_GS_CLOUD_CLEAN                      = 85,
};

enum class FaultType {
    SERVICE_FAULT = 0,
    RUNTIME_FAULT = 1,
    DATABASE_FAULT = 2,
    COMM_FAULT = 3,
};

enum class BehaviourType {
    DATABASE_BACKUP = 0,
    DATABASE_RECOVERY = 1,
};

enum class SecurityInfo {
    PERMISSIONS_APPID_FAILE = 0,
    PERMISSIONS_DEVICEID_FAILE = 1,
    PERMISSIONS_TOKENID_FAILE = 2,
    SENSITIVE_LEVEL_FAILE = 3,
};

struct ArkDataFaultMsg {
    std::string faultType;
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string businessType;
    int32_t errorType;
    std::string appendixMsg;
};

struct BehaviourMsg {
    std::string userId;
    std::string appId;
    std::string storeId;
    BehaviourType behaviourType;
    std::string extensionInfo;
};

struct UdmfBehaviourMsg {
    std::string appId;
    std::string channel;
    int64_t dataSize;
    std::string dataType;
    std::string operation;
    std::string result;
};

enum class ReportStatus {
    SUCCESS = 0,
    ERROR = 1,
};
}  // namespace DistributedDataDfx
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_DFX_TYPES_H
