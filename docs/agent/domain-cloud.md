# 云同步领域知识

## 适用范围

本文档描述设备云同步的领域知识，是云同步相关变更时的官方参考。

## 云同步架构概览

```
应用层 (CloudServiceImpl IPC)
  │
  ▼
SyncManager (云同步调度核心)
  │  SyncInfo: 同步参数
  │  NetworkSyncStrategy: 网络策略链
  │  NetworkRecoveryManager: 断网恢复补偿
  │
  ▼
EventCenter (事件驱动)
  │  CloudEvent::CLOUD_SYNC
  │  SyncEvent 携带 StoreInfo + EventInfo
  │
  ▼
AutoCache::GetStore() → GeneralStore
  │
  ▼
GeneralStore::Sync(devices, query, async, syncParam)
  │  调用 Bind() 绑定 CloudDB + AssetLoader
  │  底层 DistributedDB 引擎执行云端同步
  │
  ▼
CloudDB + AssetLoader (云驱动抽象接口)
  │
  ▼
CloudServer (云驱动注册入口)
```

## CloudServer（云驱动注册）

单例模式，使用 `RegisterCloudInstance()` 注入具体云驱动实现。

关键方法：

| 方法 | 说明 |
|------|------|
| `GetServerInfo(userId)` | 获取用户云信息（CloudInfo） |
| `GetAppSchema(userId, bundleName)` | 获取应用 Schema |
| `ConnectCloudDB(bundleName, user, dbMeta)` | 创建 CloudDB 连接 |
| `ConnectAssetLoader(bundleName, user, dbMeta)` | 创建 AssetLoader 连接 |
| `ConnectSharingCenter(userId, bundleName)` | 创建共享中心 |
| `Subscribe / Unsubscribe` | 云端订阅 |

## CloudDB（云端数据库接口）

定义在 `framework/include/cloud/cloud_db.h`，由具体云驱动实现：

| 方法 | 说明 |
|------|------|
| `BatchInsert / BatchUpdate / BatchDelete` | 批量数据操作 |
| `Query` | 查询数据 |
| `Lock / Unlock` | 云端锁定/解锁 |
| `Heartbeat` | 心跳保活 |
| `PreSharing` | 预共享 |
| `GetEmptyCursor / HasCloudUpdate` | 状态查询 |

## AssetLoader（资产加载器）

定义在 `framework/include/cloud/asset_loader.h`：

| 方法 | 说明 |
|------|------|
| `Download(tableName, gid, prefix, assets)` | 下载云端资产 |
| `RemoveLocalAssets()` | 删除本地资产 |
| `CancelDownload()` | 取消下载 |

## SchemaMeta（Schema 元数据）

三级结构：`SchemaMeta → Database → Table → Field`

### 云同步内部字段

| 字段名 | 常量 | 说明 |
|--------|------|------|
| `#_gid` | GID_FIELD | 全局唯一标识 |
| `#_createTime` | CREATE_FIELD | 创建时间 |
| `#_modifyTime` | MODIFY_FIELD | 修改时间 |
| `#_cursor` | CURSOR_FIELD | 增量同步位置标识 |
| `#_deleted` | DELETE_FIELD | 删除标记 |

### 自动同步类型

| 类型 | 说明 |
|------|------|
| `IS_NOT_AUTO_SYNC` | 不自动同步 |
| `SYNC_ON_CHANGE` | 数据变更时触发 |
| `SYNC_ON_READY` | 设备就绪时触发 |
| `SYNC_ON_CHANGE_READY` | 变更和就绪都触发 |

## CloudInfo（云用户信息）

| 字段 | 说明 |
|------|------|
| `user` | 用户 ID |
| `id` | 云用户 ID |
| `totalSpace / remainSpace` | 空间配额 |
| `enableCloud` | 云端开关 |
| `apps: map<string, AppInfo>` | 每个应用的云信息（bundleName, appId, version, cloudSwitch） |

## SyncManager 同步流程

### 主流程

```
DoCloudSync(SyncInfo)
  → 提交到线程池 GetSyncTask()
    → PrepareForCloudSync()       # 验证 CloudInfo、网络、用户状态
    → GetSchemaMeta()              # 获取 SchemaMeta
    → 注册 GetSyncHandler()        # 处理 CloudEvent::CLOUD_SYNC
    → GetPostEventTask()           # 对每个 schema.database 发送 SyncEvent

GetSyncHandler() 处理 SyncEvent:
  → GetMetaData() → GetStore()    # 获取 GeneralStore
  → StartCloudSync()              # store->Sync(devices={DEFAULT_ID}, ...)
```

### 重试机制

失败时使用 `Retryer` 机制重试（最多 6 次），根据错误码决定重试间隔。

### 补偿同步

`NetworkRecoveryManager` 在断网时记录应用列表，网络恢复后自动触发补偿同步。

## 网络策略

`NetworkSyncStrategy` 链式检查：

- WiFi 可用 → 允许同步
- Cellular 可用 → 根据配置决定
- 无网络 → 记录待补偿列表

## RDB 云同步适配

RDB 的云同步使用双适配器模式：

```
RdbCloud (继承 DistributedDB::ICloudDb)
  → 适配框架层 CloudDB 到底层 DB 接口

RdbAssetLoader (继承 DistributedDB::IAssetLoader)
  → 适配框架层 AssetLoader 到底层资产下载接口

RdbCloudConflictHandler (继承 DistributedDB::ICloudConflictHandler)
  → 适配框架层冲突处理
```

数据转换：`RdbCloudDataTranslate` 处理 Asset/Blob 序列化。

## SyncConfig（同步配置）

支持按 database 和 table 粒度控制云同步开关（`cloudSyncEnabled`），`CloudDbSyncConfig` 持久化存储在 metadata 中。

## 数据共享（共享中心）

支持将数据共享给其他用户：

| 操作              | 说明 |
|-----------------|------|
| Share           | 共享数据给其他用户 |
| Unshare         | 取消共享 |
| Exit            | 退出共享 |
| ChangePrivilege | 修改权限 |

参与者角色：`ROLE_INVITER`（邀请者）/ `ROLE_INVITEE`（被邀请者）

确认状态：`CFM_ACCEPTED / REJECTED / SUSPENDED / UNAVAILABLE`

权限控制：`writable, readable, creatable, deletable, shareable`

## 云同步约束

- 云同步需要 `ohos.permission.CLOUDDATA_CONFIG` 权限
- Schema 变更需要版本兼容处理
- 资产下载是异步的，需要处理网络中断
- 数据共享需要处理参与者状态变更
- 冲突处理遵循 CloudConflictHandler 接口
- 网络恢复后自动补偿同步，不丢失请求
