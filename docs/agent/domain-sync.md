# 同步领域知识

## 适用范围

本文档描述设备间数据同步的领域知识，是跨设备同步行为变更时的官方参考。涵盖 KVDB、RDB、Object 三个 Feature 的设备间同步机制。

## 同步模式

### 设备间同步模式

| 模式 | 值 | 说明 |
|------|---|------|
| NEARBY_PUSH | 推送本地数据到远端 | 单向 |
| NEARBY_PULL | 从远端拉取数据 | 单向 |
| NEARBY_PULL_PUSH | 双向同步 | 推拉同时进行 |

### 高位同步模式

| 模式 | 值 | 说明 |
|------|---|------|
| MANUAL_SYNC_MODE | 0x00000 | 手动触发同步 |
| AUTO_SYNC_MODE | 0x10000 | 自动同步（设备上线/数据变更触发） |
| ASSETS_SYNC_MODE | 0x20000 | 资产同步（文件传输） |

### 自动同步触发

- **设备上线**：收到设备上线通知后自动触发
- **数据变更**：本地数据写入后使用 `NotifyDataChange` 触发
- `SyncManager` 中的 `autoSyncType`：
  - `SYNC_ON_CHANGE`：数据变更时触发
  - `SYNC_ON_READY`：设备就绪时触发
  - `SYNC_ON_CHANGE_READY`：变更和就绪都触发

## KVDB 同步机制

### 核心类

| 类 | 职责 |
|---|---|
| `KVDBServiceImpl` | IPC 入口，管理 SyncAgent（每个 token 对应一个） |
| `KVDBGeneralStore` | 继承 `GeneralStore`，封装 `KvStoreNbDelegate` |
| `KvStoreSyncManager` | 单例，同步操作调度，维护三组队列 |

### 同步调度队列

`KvStoreSyncManager` 管理三组操作队列：

| 队列 | 用途 |
|------|------|
| `realtimeSyncingOps_` | 实时同步操作 |
| `delaySyncingOps_` | 延迟同步操作 |
| `scheduleSyncOps_` | 定时同步操作 |

### 手动同步流程

```
KVDBServiceImpl::Sync()
  → 获取 StoreMetaData，计算延迟时间
  → 如果是自动同步且 seqId == MAX，通知 DeviceMatrix::OnChanged()
  → KvStoreSyncManager::AddSyncOperation()
    → DoSyncInOrder()
      → DoSyncBegin()
        → KVDBGeneralStore::Sync()
          → KvStoreNbDelegate::Sync() (底层 DistributedDB)
```

### 自动同步流程

```
KVDBServiceImpl::NotifyDataChange()
  → 检查 StoreMetaData
  → 若支持 DeviceMatrix 且为 statics/dynamic 类型 → DeviceMatrix::OnChanged()
  → 若 cloudAutoSync 为真 → 延迟调度 DoCloudSync()
```

## RDB 同步机制

### 核心类

| 类 | 职责 |
|---|---|
| `RdbServiceImpl` | IPC 入口，管理 SyncAgent |
| `RdbGeneralStore` | 继承 `GeneralStore`，封装 `RelationalStoreDelegate` + `NativeRdb::RdbStore` |
| `RdbFlowControlManager` | 流控管理，限制同步频率 |

### 与 KVDB 的关键差异

| 维度 | KVDB | RDB |
|------|------|-----|
| 底层数据库 | `KvStoreNbDelegate`（键值对） | `RelationalStoreDelegate`（关系型表） |
| Schema | 无严格 schema | 有严格 schema（`SchemaMeta` 定义） |
| 分布式表 | 基本支持 | 支持 `COLLABORATION` 和 `SPLIT_BY_DEVICE` 两种模式 |
| 远程查询 | 不支持 | 支持 `RemoteQuery()` 跨设备 SQL 查询 |
| 同步调度 | `KvStoreSyncManager` 独立调度 | 直接在 ServiceImpl 中调度，有流控 |
| 流控 | 无 | 单应用5次，全局20次 |
| 资产同步 | 不涉及 | `RdbAssetLoader` 管理资产 |

## 分布式对象同步

### 核心类

| 类 | 职责 |
|---|---|
| `ObjectServiceImpl` | IPC 入口 |
| `ObjectStoreManager` | 单例，核心管理器，管理 KV 数据库 |
| `ObjectSnapshot` | 继承 `Snapshot`，管理资产状态机 |
| `ObjectAssetLoader` | 单例，管理资产跨设备传输 |
| `ObjectAssetMachine` | DFA 状态机，处理资产事件 |
| `AssetSyncManager` | 使用 DistributedFile 服务传输资产 |

### 对象生命周期

1. **Save**：写入 KV 数据 → 数据键格式 `{appId}_{sessionId}_{localUdid}_{targetDeviceId}_{timestamp}_{propertyName}` → 触发 KV 同步 → 如有资产则 `TransferAssetsAsync()`
2. **Retrieve**：从 KV 数据库读取
3. **RevokeSave**：删除指定前缀数据
4. **BindAsset**：绑定资产到对象，创建 `ObjectSnapshot`
5. **OnAssetChanged**：`ObjectAssetMachine::DFAPostEvent()` 驱动状态机

### 资产状态机

```
STATUS_STABLE → STATUS_TRANSFERRING → STATUS_UPLOADING / STATUS_DOWNLOADING
              → STATUS_WAIT_TRANSFER / STATUS_WAIT_UPLOAD / STATUS_WAIT_DOWNLOAD
```

状态恢复流程：`NONE → DATA_READY → DATA_NOTIFIED → ASSETS_READY → ALL_READY`

## 通信适配层

### 架构

```
CommunicationProvider (接口)
  → CommunicationProviderImpl
    → AppPipeMgr (管道管理器)
      → AppPipeHandler (单个管道处理器)
        → SoftBusAdapter
          → SoftBusClient (Socket 连接管理)
```

### QoS 类型

| 类型 | 说明 |
|------|------|
| QOS_BR | 蓝牙传输 |
| QOS_HML | 高速传输 |
| QOS_REUSE | 复用已有连接 |

### 在同步中的角色

- 设备间同步：DistributedDB 引擎使用 `CommunicationProvider` 收发数据
- 广播：`CommunicationProvider::Broadcast()` 用于 `DeviceMatrix` 广播变更通知
- 连接复用：`ReuseConnect()` 复用已有 BR 链路

## DeviceMatrix（设备矩阵）

### 核心概念

单例，管理设备间数据变更通知矩阵，使用**掩码（mask）** 标识每个应用的同步级别。

### 数据结构

```
DataLevel = { dynamic(16bit), statics(16bit), switches(32bit), switchesLen(16bit) }
LevelType: STATICS (系统应用) / DYNAMIC (第三方应用)
ChangeType: CHANGE_LOCAL(0x1), CHANGE_REMOTE(0x2), CHANGE_ALL(0x3)
```

### 工作流程

1. `Initialize()`：注册静态/动态应用列表，初始化本地 `MatrixMetaData`
2. `Online(device)` / `Offline(device)`：设备上下线时更新映射
3. `OnChanged(metaData)`：本地数据变更时设置对应掩码位
4. `Broadcast(dataLevel)`：使用 `CommunicationProvider::Broadcast()` 广播
5. `OnBroadcast(device, networkId, dataLevel)`：接收远程广播，更新远端映射
6. `OnExchanged(device, code, type)`：收到变更通知后触发同步

### 版本兼容

当前版本 `CURRENT_VERSION=3`，使用 `ConvertDynamic()` / `ConvertStatics()` 处理跨版本兼容。

## 克隆/备份恢复

### CloneManager

| 操作 | 说明 |
|------|------|
| `OnBackup()` | 收集密钥元数据，加密后保存 |
| `OnRestore()` | 解析备份文件，恢复密钥 |
| `ReEncryptKey()` | 使用克隆密钥重新加密数据库密钥 |

导出符号：`DistributedDataOnBackup` / `DistributedDataOnRestore`

## 同步相关约束

- 仅支持最终一致性，不支持强一致性
- 冲突解决策略基于时间戳，取较大值，不支持自定义
- 设备协同数据库：Key 拼接 DeviceID，不可修改远端数据
- 单版本数据库：每 Key 只保留一个条目
- `PermitDelegate::SyncActivate()` 验证用户是否活跃
- `PermitDelegate::IsTransferAllowed()` 检查数据流出约束
- instanceId != 0 的克隆应用禁止同步
