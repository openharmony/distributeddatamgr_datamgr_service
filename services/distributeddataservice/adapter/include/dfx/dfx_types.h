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

#include "db_meta_callback_delegate.h"

namespace OHOS {
namespace DistributedDataDfx {
struct ModuleName {
    static const inline std::string DEVICE = "DEVICE";
    static const inline std::string USER = "USER";
    static const inline std::string RDB_STORE = "RDB_STORE";
    static const inline std::string KV_STORE = "KV_STORE";
    static const inline std::string CLOUD_SERVER = "CLOUD_SERVER";
};

enum class Fault {
    // Service Fault
    SF_SERVICE_PUBLISH = 0,
    SF_GET_SERVICE = 1,
    SF_CREATE_DIR = 2,
    SF_DATABASE_CONFIG = 3,
    SF_PROCESS_LABEL = 4,
    SF_INIT_DELEGATE = 5,

    // Runtime Fault
    RF_OPEN_DB = 20,
    RF_CLOSE_DB = 21,
    RF_DELETE_DB = 22,
    RF_DATA_SUBSCRIBE = 23,
    RF_DATA_UNSUBSCRIBE = 24,
    RF_CREATE_SNAPSHOT = 25,
    RF_RELEASE_SNAPSHOT = 26,
    RF_KEY_STORE_DAMAGE = 27,
    RF_KEY_STORE_LOST = 28,
    RF_DB_ROM = 29,
    RF_GET_DB = 30,
    RF_SUBCRIBE =  31,
    RF_UNSUBCRIBE =  32,
    RF_REPORT_THERAD_ERROR = 33,

    // Communication Fault
    CF_CREATE_SESSION = 40,
    CF_CREATE_PIPE = 41,
    CF_SEND_PIPE_BROKEN = 42,
    CF_REV_PIPE_BROKEN = 43,
    CF_SERVICE_DIED = 44,
    CF_CREATE_ZDN = 45,
    CF_QUERY_ZDN = 46,

    // Database Fault
    DF_DB_DAMAGE = 60,
    DF_DB_REKEY_FAILED = 61,
    DF_DB_CORRUPTED = 62,

    // Cloud Sync Fault
    CSF_CLOUD_INFO_ENABLE_CLOUD             = 70,
    CSF_CLOUD_INFO_DISABLE_CLOUD            = 71,
    CSF_CLOUD_INFO_OPEN_CLOUD_SWITCH        = 72,
    CSF_CLOUD_INFO_CLOSE_CLOUD_SWITCH       = 73,
    CSF_CLOUD_INFO_QUERY_SYNC_INFO          = 74,
    CSF_CLOUD_INFO_USER_CHANGED             = 75,
    CSF_CLOUD_INFO_USER_UNLOCKED            = 76,
    CSF_CLOUD_INFO_NETWORK_RECOVERY         = 77,
    CSF_CLOUD_INFO_CLOUD_SYNC_TASK          = 78,
    CSF_CLOUD_INFO_SUBSCRIBE                = 79,
    CSF_CLOUD_INFO_UNSUBSCRIBE              = 80,
    CSF_BRIEF_INFO_ENABLE_CLOUD             = 81,
    CSF_BRIEF_INFO_OPEN_CLOUD_SWITCH        = 83,
    CSF_BRIEF_INFO_USER_CHANGED              = 84,
    CSF_BRIEF_INFO_USER_UNLOCKED            = 85,
    CSF_BRIEF_INFO_NETWORK_RECOVERY         = 86,
    CSF_BRIEF_INFO_CLOUD_SYNC_TASK          = 87,
    CSF_APP_SCHEMA_ENABLE_CLOUD             = 88,
    CSF_APP_SCHEMA_OPEN_CLOUD_SWITCH        = 89,
    CSF_APP_SCHEMA_QUERY_SYNC_INFO          = 90,
    CSF_APP_SCHEMA_USER_CHANGED             = 91,
    CSF_APP_SCHEMA_USER_UNLOCKED            = 92,
    CSF_APP_SCHEMA_NETWORK_RECOVERY         = 93,
    CSF_APP_SCHEMA_CLOUD_SYNC_TASK          = 94,
    CSF_CONNECT_CLOUD_ASSET_LOADER          = 95,
    CSF_CONNECT_CLOUD_DB                    = 96,
    CSF_BATCH_INSERT_GENERATE_ID            = 97,
    CSF_BATCH_INSERT_SAVE_RECORDS           = 98,
    CSF_BATCH_UPDATE_SAVE_RECORDS           = 99,
    CSF_BATCH_DELETE_DELETE_RECORDS         = 100,
    CSF_BATCH_QUERY_START_CURSOR            = 101,
    CSF_BATCH_QUERY_FETCH_RECORDS           = 102,
    CSF_BATCH_QUERY_FETCH_DB_CHANGES        = 103,
    CSF_BATCH_QUERY_FETCH_RECORD_WITH_ID    = 104,
    CSF_LOCK_GET_LOCK                       = 105,
    CSF_LOCK_HEART_BEAT                     = 106,
    CSF_SHARE_SET_PRESHARE                  = 107,
    CSF_DOWNLOAD_ASSETS                     = 108,
    CSF_GS_CREATE_DISTRIBUTED_TABLE         = 109,
    CSF_GS_SET_DISTRIBUTED_TABLE            = 110,
    CSF_GS_RDB_CLOUD_SYNC                   = 111,
    CSF_GS_KVDB_CLOUD_SYNC                  = 112,
};

enum class FaultType {
    SERVICE_FAULT = 0,
    RUNTIME_FAULT = 1,
    DATABASE_FAULT = 2,
    COMM_FAULT = 3,
    CLOUD_SYNC_FAULT = 4,
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

enum class CloudSyncScene {
    ENABLE_CLOUD = 0,
    DISABLE_CLOUD = 1,
    SWITCH_ON = 2,
    SWITCH_OFF = 3,
    QUERY_SYNC_INFO = 4,
    USER_CHANGE = 5,
    USER_UNLOCK = 6,
    NETWORK_RECOVERY = 7,
    CLOUD_SYNC_TASK = 8,
    SUBSCRIBE = 9,
    UNSUBSCRIBE = 10,
    SERVICE_INIT = 11,
    ACCOUNT_STOP = 12,
};

static std::map<CloudSyncScene, Fault> userInfoErrs = {
    { CloudSyncScene::ENABLE_CLOUD, Fault::CSF_CLOUD_INFO_ENABLE_CLOUD },
    { CloudSyncScene::DISABLE_CLOUD, Fault::CSF_CLOUD_INFO_DISABLE_CLOUD },
    { CloudSyncScene::SWITCH_ON, Fault::CSF_CLOUD_INFO_OPEN_CLOUD_SWITCH },
    { CloudSyncScene::SWITCH_OFF, Fault::CSF_CLOUD_INFO_CLOSE_CLOUD_SWITCH },
    { CloudSyncScene::QUERY_SYNC_INFO, Fault::CSF_CLOUD_INFO_QUERY_SYNC_INFO },
    { CloudSyncScene::USER_CHANGE, Fault::CSF_CLOUD_INFO_USER_CHANGED },
    { CloudSyncScene::USER_UNLOCK, Fault::CSF_CLOUD_INFO_USER_UNLOCKED },
    { CloudSyncScene::NETWORK_RECOVERY, Fault::CSF_CLOUD_INFO_NETWORK_RECOVERY },
    { CloudSyncScene::CLOUD_SYNC_TASK, Fault::CSF_CLOUD_INFO_CLOUD_SYNC_TASK },
    { CloudSyncScene::SUBSCRIBE, Fault::CSF_CLOUD_INFO_SUBSCRIBE },
    { CloudSyncScene::UNSUBSCRIBE, Fault::CSF_CLOUD_INFO_UNSUBSCRIBE }
};

static std::map<CloudSyncScene, Fault> briefInfoErrs = {
    { CloudSyncScene::ENABLE_CLOUD, Fault::CSF_BRIEF_INFO_ENABLE_CLOUD },
    { CloudSyncScene::SWITCH_ON, Fault::CSF_BRIEF_INFO_OPEN_CLOUD_SWITCH },
    { CloudSyncScene::USER_CHANGE, Fault::CSF_BRIEF_INFO_USER_CHANGED },
    { CloudSyncScene::USER_UNLOCK, Fault::CSF_BRIEF_INFO_USER_UNLOCKED },
    { CloudSyncScene::NETWORK_RECOVERY, Fault::CSF_BRIEF_INFO_NETWORK_RECOVERY },
    { CloudSyncScene::CLOUD_SYNC_TASK, Fault::CSF_BRIEF_INFO_CLOUD_SYNC_TASK }
};

static std::map<CloudSyncScene, Fault> appSchemaErrs = {
    { CloudSyncScene::ENABLE_CLOUD, Fault::CSF_APP_SCHEMA_ENABLE_CLOUD },
    { CloudSyncScene::SWITCH_ON, Fault::CSF_APP_SCHEMA_OPEN_CLOUD_SWITCH },
    { CloudSyncScene::QUERY_SYNC_INFO, Fault::CSF_APP_SCHEMA_QUERY_SYNC_INFO },
    { CloudSyncScene::USER_CHANGE, Fault::CSF_APP_SCHEMA_USER_CHANGED },
    { CloudSyncScene::USER_UNLOCK, Fault::CSF_APP_SCHEMA_USER_UNLOCKED },
    { CloudSyncScene::NETWORK_RECOVERY, Fault::CSF_APP_SCHEMA_NETWORK_RECOVERY },
    { CloudSyncScene::CLOUD_SYNC_TASK, Fault::CSF_APP_SCHEMA_CLOUD_SYNC_TASK }
};

struct FaultMsg {
    FaultType faultType;
    std::string moduleName;
    std::string interfaceName;
    Fault errorType;
};

struct DBFaultMsg {
    std::string appId;
    std::string storeId;
    std::string moduleName;
    Fault errorType;
};

struct CommFaultMsg {
    std::string userId;
    std::string appId;
    std::string storeId;
    std::vector<std::string> deviceId;
    std::vector<int32_t> errorCode;
};

struct AppendixMsg {
    uint32_t tokenId;
    int32_t uid;
    std::string GetMessage() const
    {
        return std::to_string(tokenId) + "/" + std::to_string(uid);
    }
};

struct ArkDataFaultMsg {
    int64_t faultTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    FaultType faultType;
    std::string bundleName;
    std::string moduleName = "datamgr_service";
    std::string storeId;
    std::string businessType;
    Fault errorType;
    AppendixMsg appendixMsg;
};

struct SecurityPermissionsMsg {
    std::string userId;
    std::string appId;
    std::string storeId;
    std::string deviceId;
    SecurityInfo securityInfo;
};

struct SecuritySensitiveLevelMsg {
    std::string deviceId;
    int deviceSensitiveLevel;
    int optionSensitiveLevel;
    SecurityInfo securityInfo;
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

struct VisitStat {
    std::string appId;
    std::string interfaceName;
    KVSTORE_API std::string GetKey() const
    {
        return appId + interfaceName;
    }
};

struct TrafficStat {
    std::string appId;
    std::string deviceId;
    int sendSize;
    int receivedSize;
    KVSTORE_API std::string GetKey() const
    {
        return appId + deviceId;
    }
};

struct DbStat {
    std::string userId;
    std::string appId;
    std::string storeId;
    int dbSize;
    std::shared_ptr<DistributedKv::DbMetaCallbackDelegate> delegate;

    KVSTORE_API std::string GetKey() const
    {
        return userId + appId + storeId;
    }
};

struct DbPerformanceStat {
    std::string appId;
    std::string storeId;
    uint64_t currentTime;
    uint64_t completeTime;
    int size;
    std::string srcDevId;
    std::string dstDevId;
};

struct ApiPerformanceStat {
    std::string interfaceName;
    uint64_t costTime;
    uint64_t averageTime;
    uint64_t worstTime;
    KVSTORE_API std::string GetKey() const
    {
        return interfaceName;
    }
};

enum class ReportStatus {
    SUCCESS = 0,
    ERROR = 1,
};
}  // namespace DistributedDataDfx
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_DFX_TYPES_H
