# 元数据领域知识

## 适用范围

本文档描述元数据系统的领域知识，是元数据或数据库生命周期变更时的官方参考。

## 元数据管理器

`MetaDataManager` 是**单例**，提供对本地 KV 存储的元数据 CRUD 操作：

| 操作 | 方法 |
|------|------|
| 保存 | `SaveMeta(key, value, isLocal)` |
| 加载 | `LoadMeta(key, value, isLocal)` |
| 删除 | `DelMeta(key, isLocal)` |
| 订阅 | `Subscribe(prefix, observer)` 前缀变更观察 |
| 同步 | `Sync(DeviceMetaSyncOption, OnComplete)` 跨设备元数据同步 |

特性：
- 本地元数据使用 LRU 缓存（64 条目）
- 支持本地和分布式（可同步）两种元数据
- 所有元数据类型继承 `Serializable`，实现 `Marshal(json&)` / `Unmarshal(const json&)`

## 元数据类型一览

### StoreMetaData（核心数据库描述）

**Key 前缀**：`KvStoreMetaData`

**版本**：`0x03000006`

| 字段 | 说明 |
|------|------|
| `appId` | 应用 ID |
| `bundleName` | 应用包名 |
| `storeId` | 数据库标识 |
| `user` | 用户 ID（"0" 为系统用户） |
| `deviceId` | 设备 UUID |
| `instanceId` | 克隆应用实例 ID（0=主应用） |
| `storeType` | 存储类型（KV: 0-9, Relational: 10-19, Object: 20-29, GDB: 30-39） |
| `securityLevel` | 安全级别（S0-S4） |
| `isEncrypt` | 是否加密 |
| `area` | 安全区域（EL0-EL5） |
| `account` | 华为账号 ID |
| `isAutoSync` | 是否自动同步 |
| `isPublic` | 是否公开 |
| `schema` | 数据库 Schema |

Key 体系：

| 方法 | 用途 |
|------|------|
| `GetKey()` | 完整元数据键 |
| `GetKeyLocal()` | 本地元数据键 |
| `GetSecretKey()` | 加密密钥键 |
| `GetStrategyKey()` | 同步策略键 |
| `GetAutoLaunchKey()` | 自动拉起键 |

### StoreMetaDataLocal（本地配置）

**Key 前缀**：`KvStoreMetaDataLocal`

| 字段 | 说明 |
|------|------|
| `isAutoSync` | 是否自动同步 |
| `isEncrypt` | 是否加密 |
| `isPublic` | 是否公开 |
| `schema` | Schema 定义 |
| `policies` | 策略数组（PolicyValue） |
| `PromiseInfo` | 授权信息（authorized tokenIds/uids/permissions） |

### StoreMetaMapping（路径映射）

扩展 `StoreMetaData`，增加路径映射：

| 字段 | 说明 |
|------|------|
| `cloudPath` | 云端路径 |
| `devicePath` | 设备路径 |
| `searchPath` | 搜索路径 |

### SecretKeyMetaData（加密密钥）

**Key 前缀**：`SecretKey` / `BackupSecretKey` / `Clone`

| 字段 | 说明 |
|------|------|
| `time` | 时间戳 |
| `sKey` | 加密后的密钥 |
| `nonce` | 随机 nonce |
| `storeType` | 存储类型 |
| `area` | 安全区域 |

### CapMetaData（设备能力）

**Key 前缀**：`CapabilityMeta`

跟踪设备能力版本（版本 3 包含 UDMF 和 Object 支持），用于设备间能力协商。

### MatrixMetaData（设备矩阵）

**Key 前缀**：`MatrixMeta`

| 字段 | 说明 |
|------|------|
| `version` | 版本号 |
| `dynamic/staticsInfo` | 动态/静态级别信息 |
| `origin` | 来源（LOCAL/REMOTE_RECEIVED/REMOTE_CONSISTENT） |
| `deviceId` | 设备 ID |

### StrategyMeta（同步策略）

**Key 前缀**：`StrategyMetaData`

| 字段 | 说明 |
|------|------|
| `devId` | 设备 ID |
| `userId` | 用户 ID |
| `bundleName` | 应用包名 |
| `storeId` | 数据库标识 |
| `capabilityEnabled` | 能力开关 |
| `capabilityRange` | 能力范围 |

### UserMetaData（用户状态）

**Key 前缀**：`UserMeta`

设备用户状态：`deviceId + list of UserStatus(id, isActive)`

### CorruptedMetaData（损坏标记）

跟踪损坏的数据库：`appId, bundleName, storeId, isCorrupted`

### AutoLaunchMetaData（自动拉起）

自动拉起配置：`datas map, launchForCleanData flag`

### SwitchesMetaData（功能开关）

**Key 前缀**：`SwitchesMeta`

设备级功能开关：`version, value bitmask, length, deviceId, networkId`

### VersionMetaData（版本追踪）

**Key 前缀**：`VersionKey`

元数据 Schema 版本，当前 `CURRENT_VERSION=5`。

## 元数据版本升级规则

新增元数据字段时的标准流程：

1. 新增字段设置合理默认值
2. 递增 `VersionMetaData::CURRENT_VERSION`
3. 在 `Unmarshal()` 中处理旧版本缺失字段的默认值
4. 确保新代码能正确处理旧版本数据
5. 不可修改已有字段的名称和语义

## 备份/恢复机制

### BackupManager

单例，支持定时备份：

| 阶段 | 说明 |
|------|------|
| `Init()` | 检查所有 store 元数据的备份状态，确定 ClearType |
| `BackSchedule()` | 使用 `ExecutorPool::Schedule()` 定时调度 |
| `DoBackup()` | 保留旧备份(.bk) → 导出新备份(.tmp) → 原子替换 |

### ClearType

| 类型 | 说明 |
|------|------|
| `DO_NOTHING` | 无需操作 |
| `ROLLBACK` | 回滚到上次备份 |
| `CLEAN_DATA` | 清理数据 |

### 备份配置

| 配置项 | 说明 |
|--------|------|
| `rules` | 备份规则名称列表 |
| `schedularDelay` | 初始延迟 |
| `schedularInternal` | 重复间隔 |
| `backupInternal` | 最小备份间隔 |
| `backupNumber` | 每批备份数量 |

配置来源：`conf/config.json` 中的 `BackupConfig`。

### BackupRuleManager

插件化规则系统：`RegisterPlugin(ruleName, getter)` 注册命名规则。所有规则必须通过 `CanBackup()` 才允许备份。

## 配置管理

### ConfigFactory

单例，加载 `/system/etc/distributeddata/conf/config.json`，解析为 `GlobalConfig`：

| 配置模型 | 用途 |
|---------|------|
| `ComponentConfig` | 动态库加载 |
| `NetworkConfig` | 网络配置 |
| `CheckerConfig` | Bundle 检查器配置 |
| `DirectoryConfig` | 目录策略配置 |
| `BackupConfig` | 备份配置 |
| `CloudConfig` | 云同步 bundle 映射 |
| `ThreadConfig` | 线程池配置 |
| `DataShareConfig` | 数据共享配置 |
| `AutoSyncAppConfig` | 应用自动同步配置 |

## Snapshot 系统

### Snapshot（抽象基类）

资产传输生命周期接口：

| 方法 | 说明 |
|------|------|
| `Transferred(asset)` | 资产已传输 |
| `Upload / Download` | 发起上传/下载 |
| `Uploaded / Downloaded` | 标记完成 |
| `BindAsset` | 绑定资产到存储位置 |
| `OnDataChanged` | 处理远程变更 |

### 转移状态

```
STATUS_STABLE → STATUS_TRANSFERRING → STATUS_UPLOADING / STATUS_DOWNLOADING
              → STATUS_WAIT_TRANSFER / STATUS_WAIT_UPLOAD / STATUS_WAIT_DOWNLOAD
```

### BindEvent

基于 `EventCenter` 的集成事件：
- `BIND_SNAPSHOT`：绑定新资产
- `COMPENSATE_SYNC`：补偿遗漏的同步
- `RECOVER_SYNC`：从同步失败中恢复
