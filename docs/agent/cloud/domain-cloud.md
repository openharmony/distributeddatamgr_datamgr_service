# 端云同步领域知识

> 本文档为 Agent 知识路由目标，由 `services/distributeddataservice/service/cloud/AGENTS.md` 的 vocabulary-based / path-based routing 触发阅读。

## 1. 核心类关系

### 1.1 端云服务核心类关系图

> 类图仅展示关键公有接口和端云相关成员，完整定义请查看对应头文件。

```mermaid
classDiagram
    class CloudServiceStub {
        +DECLARE_INTERFACE_DESCRIPTOR OHOS.CloudData.CloudServer
        +OnRemoteRequest(code, data, reply) int
        +HANDLERS TRANS_BUTT Handler[]
    }
    class CloudServiceImpl {
        +EnableCloud / DisableCloud / ChangeAppSwitch
        +Clean / NotifyDataChange
        +QueryStatistics / QueryLastSyncInfo / QueryLastSyncInfoBatch
        +SetGlobalCloudStrategy / CloudSync / StopCloudSyncTask / StopCloudSyncForStore / InitNotifier
        +Subscribe / Unsubscribe / SubscribeCloudSyncTrigger / UnSubscribeCloudSyncTrigger
        +AllocResourceAndShare / Share / Unshare / Exit / ChangePrivilege / Query / QueryByInvitation / ConfirmInvitation / ChangeConfirmation
        +SetCloudStrategy
        +OnInitialize / OnBind / OnUserChange / OnReady / Offline / OnScreenUnlocked
        -RETRY_TIMES 3 / EXPIRE_INTERVAL 2*24 / NOTIFY_DELAY 5s
    }
    class SyncManager {
        +DoCloudSync SyncInfo
        +StopCloudSync user
        +QueryLastSyncInfo
        +static GetStore meta user mustBind
        +Bind executor
        +OnNetworkDisconnected / OnNetworkConnected / OnScreenUnlocked
        +CleanCompensateSync / ClearLastSyncInfo
        +static Report / static GetPath
    }
    class CloudServer {
        +static GetInstance CloudServer
        +static RegisterCloudInstance instance bool
        +GetServerInfo userId needSpaceInfo pair~int32_t CloudInfo~
        +GetAppSchema userId bundleName pair~int32_t SchemaMeta~
        +Subscribe userId dbs int32_t
        +Unsubscribe userId dbs int32_t
        +ConnectAssetLoader 2重载 shared_ptr~AssetLoader~
        +ConnectCloudDB 2重载 shared_ptr~CloudDB~
        +ConnectSharingCenter userId bunleName shared_ptr~SharingCenter~
        +Clean userId / ReleaseUserInfo userId / Bind executor
        +IsSupportCloud userId bool / CloudDriverUpdated bundleName bool
        +GetConflictHandler shared_ptr~CloudConflictHandler~
    }
    class CloudDB {
        +virtual Execute / BatchInsert / BatchUpdate 2重载 / BatchDelete
        +virtual Query 2重载 / PreSharing
        +virtual Sync / StopCloudSync
        +virtual Watch / Unwatch / Lock / Heartbeat / Unlock
        +virtual AliveTime int64_t / Close int32_t / GetEmptyCursor / SetPrepareTraceId / HasCloudUpdate bool
    }
    class CloudInfo {
        +DEFAULT_BATCH_NUMBER 30 / DEFAULT_BATCH_SIZE 1.5M
        +AppInfo bundleName appId version instanceId cloudSwitch
        +user id totalSpace remainSpace maxNumber maxSize enableCloud apps
        +GetKey / GetSchemaKey / GetSchemaPrefix / IsValid / Exist / IsOn / IsAllSwitchOff / GetAppInfo
    }
    class SchemaMeta {
        +14个系统字段常量
        +CURRENT_VERSION 0x10001 / CLEAN_WATER_VERSION 0x10001
        +metaVersion version bundleName databases e2eeEnable
        +GetLowVersion / GetHighVersion / IsValid / GetDataBase / GetStores
    }
    class Database {
        +name alias tables autoSyncType user deviceId version bundleName
        +GetKey / GetPrefix / GetTableNames / GetSyncTables / GetCloudTables
    }
    class Table {
        +name sharedTableName alias fields cloudSyncFields
    }
    class Field {
        +colName alias type primary autoIncrement nullable dupCheckCol
    }
    class Subscription {
        +userId id expiresTime
        +Relation id bundleName relations
        +GetKey / GetRelationKey / GetMinExpireTime / GetPrefix
    }
    class SharingCenter {
        +Role / Confirmation / SharingCode枚举
        +Privilege / Participant结构
        +Participants / Results / QueryResults类型
        +SHARING_ERR_OFFSET
        +所有方法均为virtual Share Unshare Exit ChangePrivilege Query QueryByInvitation ConfirmInvitation ChangeConfirmation
    }
    class GeneralStore {
        +端云核心枚举结构详见 general_store.h
        +Sync / StopCloudSync / Bind / IsBound / Clean / LockCloudDB / UnLockCloudDB
        +SetCloudConflictHandler / PreSharing / SetConfig / SetEqualIdentifier / BindSnapshots
        +UpdateDBStatus / SetCacheFlag / PublishCacheChange
    }
    class AutoCache {
        +StoreOption createRequired false
        +static GetInstance
        +GetStore / GetDBStore
    }
    class AssetLoader {
        +Download 2重载
        +RemoveLocalAssets / CancelDownload / SetStoreMetaKey
    }
    class CloudConflictHandler {
        +HandleConflict table oldData newData upsert int32_t
    }

    CloudServiceStub <|-- CloudServiceImpl
    CloudServiceImpl --> SyncManager : 使用
    CloudServiceImpl --> CloudServer : 获取云组件
    SyncManager --> GeneralStore : 执行同步
    SyncManager --> CloudDB : 云端操作
    CloudServer --> CloudDB : 创建
    CloudServer --> SharingCenter : 创建
    CloudServer --> AssetLoader : 创建
    GeneralStore --> AutoCache : 缓存管理
    GeneralStore --> CloudConflictHandler : 使用
    SchemaMeta *-- Database : 包含
    Database *-- Table : 包含
    SharingCenter --> Participant : 使用
```

### 1.2 端云同步相关类图

```mermaid
classDiagram
    class SyncInfo {
        +Param user bundleName store tables triggerMode prepareTraceId
        +SyncInfo 4个构造重载
        +SetMode / SetWait / SetAsyncDetail / SetQuery / SetError / SetCompensation
        +SetTriggerMode / SetPrepareTraceId / SetDownloadOnly / SetEnablePredicate
        +GenerateQuery / Contains / GetTables / IsAutoSync
    }
    class CloudLastSyncInfo {
        +id storeId startTime finishTime
        +code syncStatus instanceId
        +Marshal / Unmarshal API_LOCAL
        +static GetKey 2公开重载 1私有重载
    }
    class Serializable {
        +Marshal json
        +Unmarshal json
    }
    class SyncEvent {
        +EventInfo 2构造重载 移动拷贝
        +GetMode / GetWait / AutoRetry / GetQuery / GetAsyncDetail / IsCompensation
        +GetTriggerMode / GetPrepareTraceId / GetUser / GetDownloadOnly / GetEnablePredicate
    }
    class CloudEvent {
        +FEATURE_INIT / GET_SCHEMA / LOCAL_CHANGE / CLEAN_DATA / CLOUD_SYNC / DATA_CHANGE
        +SET_SEARCHABLE / CLOUD_SHARE / MAKE_QUERY / CLOUD_SYNC_FINISHED / DATA_SYNC
        +LOCK_CLOUD_CONTAINER / UNLOCK_CLOUD_CONTAINER / SET_SEARCH_TRIGGER / UPGRADE_SCHEMA / DATABASE_DELETED / CLOUD_BUTT
        +GetStoreInfo / GetEventId
    }
    class Event {
        +eventId storeInfo
    }
    class NetworkRecoveryManager {
        +OnNetworkDisconnected / OnNetworkConnected / RecordSyncApps
    }
    class NetworkSyncStrategy {
        +Strategy WIFI 0x01 / CELLULAR 0x02 / BUTT
        +StrategyInfo user bundleName strategy Marshal Unmarshal operator== GetKey
        +CheckSyncAction storeInfo int32_t
        +static GetKey 2重载
    }
    class SyncStrategy {
        +CheckSyncAction storeInfo int32_t
        +SetNext next shared_ptr~SyncStrategy~
        +GLOBAL_BUNDLE GLOBAL
    }
    class CloudConflictHandler {
        +HandleConflict table oldData newData upsert int32_t
    }

    CloudLastSyncInfo --|> Serializable : 继承
    SyncEvent --|> CloudEvent : 继承
    CloudEvent --|> Event : 继承
    NetworkSyncStrategy --|> SyncStrategy : 继承
    SyncManager *-- NetworkRecoveryManager : 包含
    SyncManager --> CloudLastSyncInfo : 管理
    SyncManager --> SyncStrategy : 使用
    SyncManager --> SyncEvent : 处理
    SyncManager --> CloudEvent : 处理
```

---

## 2. 时序图

### 2.1 端云同步时序图

```mermaid
sequenceDiagram
    participant App as 应用层
    participant CloudService as CloudServiceImpl
    participant EventCenter as EventCenter
    participant SyncManager as SyncManager
    participant CloudServer as CloudServer
    participant GeneralStore as GeneralStore
    participant CloudDB as CloudDB
    participant AssetLoader as AssetLoader
    participant Cloud as 云服务器

    App->>CloudService: CloudSync(bundleName, storeId, option, async)
    CloudService->>EventCenter: PostEvent(ChangeEvent)
    EventCenter->>SyncManager: GetSyncTask()

    SyncManager->>SyncManager: PrepareForCloudSync()
    alt NeedGetCloudInfo
        SyncManager->>CloudServer: GetServerInfo(userId)
        CloudServer-->>SyncManager: CloudInfo
    end

    alt SchemaMeta无效
        SyncManager->>EventCenter: PostEvent(GET_SCHEMA)
        EventCenter->>CloudService: GetSchema()
        CloudService->>CloudServer: GetAppSchema(userId, bundleName)
        CloudServer->>Cloud: 获取Schema
        Cloud-->>CloudServer: SchemaMeta
        CloudServer-->>CloudService: SchemaMeta
        CloudService->>CloudService: SaveMeta(SchemaMeta)
        SyncManager->>SyncManager: LoadMeta(SchemaMeta)
    end

    SyncManager->>SyncManager: GetStore()
    SyncManager->>CloudServer: ConnectCloudDB(bundleName, user, dbMeta)
    CloudServer-->>SyncManager: CloudDB实例
    SyncManager->>CloudServer: ConnectAssetLoader(bundleName, user)
    CloudServer-->>SyncManager: AssetLoader实例
    SyncManager->>GeneralStore: Bind(bindInfos{db, loader, accountId}, cloudConfig)
    GeneralStore-->>SyncManager: E_OK

    SyncManager->>GeneralStore: Sync({DEFAULT_ID}, query, asyncCallback, syncParam)
    GeneralStore->>CloudDB: Sync(devices, mode, query, async, wait)

    CloudDB->>Cloud: 上传本地变更数据
    Cloud-->>CloudDB: 上传结果
    CloudDB->>Cloud: 同步数据
    Cloud-->>CloudDB: 下载数据

    CloudDB-->>GeneralStore: 同步结果(async回调)
    GeneralStore-->>SyncManager: asyncCallback(GenDetails)

    SyncManager->>SyncManager: SaveLastSyncInfo()
    CloudService->>App: CloudNotifierProxy.OnComplete(GenDetails)
```

### 2.2 端云订阅时序图

**流程A：应用订阅通知（本地记录）**

```mermaid
sequenceDiagram
    participant App as 应用层
    participant CloudService as CloudServiceImpl

    App->>CloudService: Subscribe(type, bundleInfos, observer)
    CloudService->>CloudService: subscribes_[type][key] = tokenId
    CloudService-->>App: SUCCESS

    App->>CloudService: Unsubscribe(type, bundleInfos, observer)
    CloudService->>CloudService: subscribes_[type] 移除tokenId
    CloudService-->>App: SUCCESS
```

**流程B：后台定时云端订阅**

```mermaid
sequenceDiagram
    participant CloudService as CloudServiceImpl
    participant CloudServer as CloudServer(Rust层)
    participant Cloud as 云服务器

    CloudService->>CloudService: DoSubscribe(user, scene)
    CloudService->>CloudServer: Subscribe(userId, subDbs)
    CloudServer->>Cloud: OhCloudExtCloudSyncSubscribe(userId, dbs, expire)
    Cloud-->>CloudServer: 订阅确认 + relations
    CloudServer-->>CloudService: Subscribe结果
```

**流程C：数据变更通知**

```mermaid
sequenceDiagram
    participant Cloud as 云服务器
    participant CloudService as CloudServiceImpl
    participant NotifierProxy as CloudNotifierProxy
    participant App as 应用层

    Cloud->>CloudService: 数据变更推送(Rust回调)
    CloudService->>CloudService: OnSyncInfoChanged(event)
    CloudService->>CloudService: 从subscribes_查找订阅者tokenIds
    CloudService->>NotifierProxy: OnSyncInfoNotify(data)
    NotifierProxy->>App: IPC通知同步信息变更
```

### 2.3 云数据共享时序图

```mermaid
sequenceDiagram
    participant App as 应用层
    participant CloudService as CloudServiceImpl
    participant EventCenter as EventCenter
    participant SyncManager as SyncManager
    participant CloudServer as CloudServer
    participant GeneralStore as GeneralStore
    participant CloudDB as CloudDB
    participant SharingCenter as SharingCenter
    participant Cloud as 云服务器

    App->>CloudService: AllocResourceAndShare(storeId, predicates, columns, participants)
    CloudService->>EventCenter: PostEvent(MakeQueryEvent)
    EventCenter->>SyncManager: GetStore()
    SyncManager->>CloudServer: ConnectCloudDB(bundleName, user, dbMeta)
    CloudServer-->>SyncManager: CloudDB实例
    SyncManager->>GeneralStore: Bind(bindInfos, cloudConfig)
    GeneralStore-->>SyncManager: store实例
    SyncManager-->>CloudService: GeneralStore实例

    CloudService->>GeneralStore: PreSharing(query)
    GeneralStore->>CloudDB: PreSharing(table, extend)
    CloudDB->>Cloud: 预分享请求
    Cloud-->>CloudDB: 分享资源ID
    CloudDB-->>GeneralStore: Cursor(sharingRes)
    GeneralStore-->>CloudService: sharingRes列表

    CloudService->>CloudServer: ConnectSharingCenter(userId, bundleName)
    CloudServer-->>CloudService: SharingCenter实例

    loop 遍历每个sharingRes
        CloudService->>SharingCenter: Share(userId, bundleName, sharingRes, participants)
        SharingCenter->>Cloud: 发送邀请
        Cloud-->>SharingCenter: 邀请结果
        SharingCenter-->>CloudService: 分享结果
    end

    loop 参与者响应(预期行为)
        Cloud->>SharingCenter: 参与者确认
        SharingCenter->>CloudService: 确认通知
        CloudService->>App: 状态更新
    end

    App->>CloudService: Query(sharingRes, results)
    CloudService->>CloudServer: ConnectSharingCenter(userId, bundleName)
    CloudServer-->>CloudService: SharingCenter实例
    CloudService->>SharingCenter: Query(userId, bundleName, sharingRes)
    SharingCenter->>Cloud: 查询分享状态
    Cloud-->>SharingCenter: 分享详情
    SharingCenter-->>CloudService: 参与者列表
```

---

## 3. 流程图

### 3.1 端云同步主流程

```mermaid
flowchart TD
    A[开始云同步] --> B{检查云开关}
    B -->|关闭| C[返回云服务未启用]
    B -->|开启| D[获取CloudInfo]
    
    D --> E{CloudInfo有效?}
    E -->|无效| F[从服务器获取CloudInfo]
    F --> G{FetchSuccess?}
    G -->|Failed| H[返回错误]
    G -->|Success| I[更新本地CloudInfo]
    
    E -->|有效| J[获取SchemaMeta]
    I --> J
    J --> K{SchemaMeta有效?}
    K -->|无效| L[从服务器获取Schema]
    L --> M{FetchSuccess?}
    M -->|Failed| H
    M -->|Success| N[更新本地SchemaMeta]
    
    K -->|有效| O[连接CloudDB]
    N --> O
    O --> P[绑定GeneralStore]
    P --> Q[准备同步任务]
    
    Q --> R{网络状态检查}
    R -->|断开| S[记录补偿同步]
    S --> T[等待网络恢复]
    T --> R
    
    R -->|连接| U[执行同步]
    U --> V[上传本地变更]
    V --> W{UploadSuccess?}
    W -->|Failed| X[处理错误]
    X --> Y{需要重试?}
    Y -->|需要| Z[加入重试队列]
    Z --> U
    Y -->|不需要| H
    
    W -->|Success| AA[同步完成]
    AA --> AB{操作成功？}
    AB -->|Failed| X
    AB -->|Success| AC[合并数据]
    
    AC --> AD{数据冲突?}
    AD -->|有冲突| AE[冲突处理]
    AE --> AF[应用冲突解决策略]
    AF --> AC
    
    AD -->|无冲突| AG[更新水印]
    AG --> AH[保存同步信息]
    AH --> AI[回调通知]
    AI --> AJ[同步完成]
```

### 3.2 数据变更通知流程

```mermaid
flowchart TD
    A[数据变更触发] --> B[NotifyDataChange]
    B --> C{检查通知条件}
    C -->|条件不满足| D[忽略通知]
    C -->|条件满足| E[获取ExtraData]
    
    E --> F[解析变更信息]
    F --> G{变更类型判断}
    
    G -->|数据库变更| H[获取DbInfo]
    H --> I[触发数据库同步]
    
    G -->|应用变更| J[获取BundleInfo]
    J --> K[触发应用级同步]
    
    I --> L[创建SyncInfo]
    K --> L
    L --> M[设置触发模式]
    M --> N[DoCloudSync]
    
    N --> O[ProcessDatabaseSync]
    O --> P{检查自动同步开关}
    P -->|关闭| Q[跳过同步]
    P -->|开启| R[触发端云同步]
    
    R --> S[NotifyCloudSyncTriggerObservers]
    S --> T[通知订阅者]
```

---

## 4. 状态机

### 4.1 端云同步状态机

```mermaid
stateDiagram-v2
    [*] --> Idle: 初始状态
    
    Idle --> Preparing: 收到同步请求
    
    Preparing --> Validating: 获取CloudInfo
    Preparing --> Error: CloudInfo无效
    
    Validating --> Binding: SchemaMeta有效
    Validating --> FetchSchema: SchemaMeta无效
    Validating --> Error: 验证失败
    
    FetchSchema --> Binding: SchemaFetchSuccess
    FetchSchema --> Retrying: Schema获取失败
    FetchSchema --> Error: 重试次数耗尽
    
    Binding --> Syncing: BindSuccess
    Binding --> Error: 绑定失败
    
    Syncing --> Uploading: 开始上传
    Syncing --> WaitingNetwork: 网络断开
    Syncing --> Stopped: 收到停止请求
    
    Uploading --> Downloading: UploadSuccess
    Uploading --> Retrying: 上传失败
    Uploading --> Error: 重试次数耗尽
    
    Downloading --> Merging: DownloadSuccess
    Downloading --> Retrying: 下载失败
    Downloading --> Error: 重试次数耗尽
    
    Merging --> ResolvingConflict: 发现冲突
    Merging --> Completing: 无冲突
    
    ResolvingConflict --> Merging: 冲突解决
    ResolvingConflict --> Error: 冲突解决失败
    
    Completing --> Idle: 同步完成
    
    WaitingNetwork --> Syncing: 网络恢复
    WaitingNetwork --> Compensate: 记录补偿同步
    
    Compensate --> Syncing: 触发补偿同步
    
    Retrying --> Syncing: 重试
    Retrying --> Error: 重试次数耗尽
    
    Stopped --> Idle: 停止完成
    
    Error --> Idle: 错误处理完成
```

### 4.2 云开关状态机

```mermaid
stateDiagram-v2
    [*] --> Disabled: 初始状态
    
    Disabled --> Enabling: EnableCloud调用
    Enabling --> Enabled: EnableSuccess
    Enabling --> Disabled: 启用失败
    
    Enabled --> ChangingAppSwitch: ChangeAppSwitch调用
    Enabled --> Disabling: DisableCloud调用
    
    ChangingAppSwitch --> Enabled: 应用开关变更完成
    Disabling --> Disabled: 禁用完成
    
    Enabled --> Cleaning: Clean调用
    Cleaning --> Enabled: 清理完成
    Cleaning --> Disabled: 清理所有数据
```

### 4.3 云订阅状态机

```mermaid
stateDiagram-v2
    [*] --> Unsubscribed: 初始状态
    
    Unsubscribed --> Subscribing: Subscribe调用
    Subscribing --> Subscribed: SubscribeSuccess
    Subscribing --> Unsubscribed: 订阅失败
    
    Subscribed --> Notifying: 收到变更通知
    Subscribed --> Unsubscribing: Unsubscribe调用
    Subscribed --> Renewing: 订阅即将过期
    
    Notifying --> Subscribed: 通知处理完成
    Renewing --> Subscribed: RenewSuccess
    Renewing --> Expired: 续订失败
    
    Unsubscribing --> Unsubscribed: 取消订阅完成
    Expired --> Subscribing: 重新订阅
    Expired --> Unsubscribed: 订阅失效
```

---

## 5. 用例图

```mermaid
graph TB
    subgraph Actors
        User[用户]
        App[应用开发者]
        System[系统服务]
    end
    
    subgraph CloudService[端云服务]
        UC1[启用/禁用云服务]
        UC2[云数据同步]
        UC3[查询同步状态]
        UC4[订阅数据变更]
        UC5[数据分享]
        UC6[清理云数据]
        UC7[设置同步策略]
        UC8[统计信息查询]
    end
    
    User --> UC2
    User --> UC5
    User --> UC3
    App --> UC1
    App --> UC2
    App --> UC3
    App --> UC4
    App --> UC5
    App --> UC6
    App --> UC7
    App --> UC8
    System --> UC1
    System --> UC6
```

---

## 6. 内部模块依赖

```mermaid
graph TD
    A[CloudServiceImpl] --> B[SyncManager]
    A --> C[CloudServer]
    A --> D[Subscription]
    A --> Q[SyncConfig]
    B --> E[GeneralStore]
    B --> F[CloudDB]
    B --> G[SyncStrategy/NetworkSyncStrategy]
    B --> H[NetworkRecoveryManager]
    E --> I[AutoCache]
    C --> F
    C --> L[AssetLoader]
    C --> M[SharingCenter]
    C --> N[CloudConflictHandler]
    F --> O[CloudInfo]
    F --> P[SchemaMeta]
    Q --> R[CloudDbSyncConfig]
```

---

## 7. 错误处理

### 7.1 SharingCenter 分享错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| SUCCESS | 0 | 成功 |
| REPEATED_REQUEST | 1 | 重复请求 |
| NOT_INVITER | 2 | 非邀请者 |
| NOT_INVITER_OR_INVITEE | 3 | 非邀请者或受邀者 |
| OVER_QUOTA | 4 | 超出配额 |
| TOO_MANY_PARTICIPANTS | 5 | 参与者过多 |
| INVALID_ARGS | 6 | 参数无效 |
| NETWORK_ERROR | 7 | 网络错误 |
| CLOUD_DISABLED | 8 | 云服务未启用 |
| SERVER_ERROR | 9 | 服务器错误 |
| INNER_ERROR | 10 | 内部错误 |
| INVALID_INVITATION | 11 | 无效邀请 |
| RATE_LIMIT | 12 | 速率限制 |
| IPC_ERROR | 13 | IPC错误 |
| NOT_SUPPORT | 14 | 不支持 |
| CUSTOM_ERROR | 1000 | 自定义错误起始值 |

### 7.2 CloudServiceImpl 常量

```cpp
static constexpr uint64_t INVALID_SUB_TIME = 0;
static constexpr int32_t RETRY_TIMES = 3;
static constexpr int32_t RETRY_INTERVAL = 60;
static constexpr int32_t EXPIRE_INTERVAL = 2 * 24;   // 2天
static constexpr int32_t WAIT_TIME = 30;
static constexpr int32_t DEFAULT_USER = 0;
static constexpr int32_t TIME_BEFORE_SUB = 12 * 60 * 60 * 1000;  // 12小时(ms)
static constexpr int32_t SUBSCRIPTION_INTERVAL = 60 * 60 * 1000;  // 1小时(ms)
static constexpr ExecutorPool::Duration NOTIFY_DELAY = std::chrono::seconds(5);
static constexpr Handle WORK_CLOUD_INFO_UPDATE = &CloudServiceImpl::UpdateCloudInfo;
static constexpr Handle WORK_SCHEMA_UPDATE = &CloudServiceImpl::UpdateSchema;
static constexpr Handle WORK_SUB = &CloudServiceImpl::DoSubscribe;
static constexpr Handle WORK_RELEASE = &CloudServiceImpl::ReleaseUserInfo;
static constexpr Handle WORK_DO_CLOUD_SYNC = &CloudServiceImpl::DoCloudSync;
static constexpr Handle WORK_STOP_CLOUD_SYNC = &CloudServiceImpl::StopCloudSync;
```

### 7.3 SyncManager 常量

```cpp
static constexpr ExecutorPool::Duration RETRY_INTERVAL = std::chrono::seconds(10);
static constexpr ExecutorPool::Duration LOCKED_INTERVAL = std::chrono::seconds(30);
static constexpr ExecutorPool::Duration BUSY_INTERVAL = std::chrono::seconds(180);
static constexpr int32_t RETRY_TIMES = 6;
static constexpr int32_t CLIENT_RETRY_TIMES = 3;
static constexpr uint64_t USER_MARK = 0xFFFFFFFF00000000;
static constexpr int32_t MV_BIT = 32;
static constexpr int32_t EXPIRATION_TIME = 6 * 60 * 60 * 1000;  // 6小时(ms)
```

---

## 8. 安全说明

### 8.1 权限要求

| 权限 | 说明 |
|------|------|
| ohos.permission.CLOUDFILE_SYNC | 云文件同步权限 |
| ohos.permission.GET_NETWORK_INFO | 获取网络信息权限 |
| ohos.permission.MONITOR_DEVICE_NETWORK_STATE | 监控设备网络状态权限(ACL) |
| ohos.permission.USE_CLOUD_DRIVE_SERVICE | 使用云盘服务权限 |

---

## 9. 配置说明

### 9.1 服务配置文件

**文件**：`services/distributeddataservice/app/distributed_data.cfg`

> 以下仅列出端云相关配置，完整配置请查看配置文件。

| 端云相关配置 | 说明 |
|-------------|------|
| permission（端云相关） | CLOUDFILE_SYNC / GET_NETWORK_INFO / MONITOR_DEVICE_NETWORK_STATE / USE_CLOUD_DRIVE_SERVICE / PUBLISH_SYSTEM_COMMON_EVENT / GET_BUNDLE_INFO / GET_BUNDLE_INFO_PRIVILEGED |
| permission_acls（端云相关） | MONITOR_DEVICE_NETWORK_STATE |

### 9.2 云同步配置

```cpp
struct CloudConfig {
    int32_t maxNumber = 30;              // 最大批次数量
    int32_t maxSize = 1024 * 512 * 3;    // 最大批次大小 1.5MB
    int32_t maxRetryConflictTimes = 3;   // 冲突重试次数
    bool isSupportEncrypt = false;       // 是否支持加密
};
```

---

## 10. API 兼容性规范

### 10.1 IPC 接口规则

- CloudServiceStub Handler 编号不得重新分配，新增只能追加。
- IPC 参数顺序变更视为破坏性变更。
- 回调语义变更（如 OnComplete 回调参数含义）影响应用层，必须明确标注。
- 权限声明变更属于兼容性敏感变更，必须经审批。

### 10.2 跨模块类型定义

- `cloud_types.h` 中的 Role/Confirmation/Privilege/Participant/Strategy 等类型由 `distributeddatamgr_relational_store` 定义。
- 变更这些类型需同步确认 relational_store 侧的兼容性。

### 10.3 兼容性检查方法

- 检查 CloudServiceStub Handler 编号是否只增不减。
- 检查 IPC 参数顺序是否与上一版本一致。
- 检查错误码是否只追加不修改。
- 检查 `cloud_types.h` 类型定义是否与 relational_store 侧对齐。
