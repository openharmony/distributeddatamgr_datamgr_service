# 安全边界

## 适用范围

本文档描述权限、身份验证、信任状态的安全边界，是权限模型、信任边界变更时的官方参考。

## 安全边界总览

| 编号 | 安全边界 | 说明 |
|------|---------|------|
| SB-1 | 用户数据隔离 | 不同 OS 用户的数据三层隔离 |
| SB-2 | 跨账户访问控制 | 跨用户访问必须持特定权限 |
| SB-3 | 读写权限强制 | ProxyData 模式禁止空权限 |
| SB-4 | 加密密钥隔离 | CE/ECE 级密钥与用户绑定 |
| SB-5 | 分布式同步权限 | 同步必须持有 DISTRIBUTED_DATASYNC |
| SB-6 | Silent Proxy 开关 | 所有 CRUD 操作前强制检查 |
| SB-7 | 白名单限制 | 白名单非空时严格匹配 |
| SB-8 | 数据流出约束 | OS 账户约束可禁止数据流出 |
| SB-9 | 同步激活验证 | 仅活跃用户数据允许同步 |
| SB-10 | 安全区域-存储级别对应 | EL2+ 必须使用 CE/ECE 级别 |
| SB-11 | 克隆应用数据隔离 | 克隆实例路径独立 |
| SB-12 | 系统应用 Extension 访问 | Extension 直通仅限系统应用 |
| SB-13 | 用户停止时数据清理 | 用户停止时立即关闭数据库连接 |
| SB-14 | Predicate 安全验证 | 无效谓词被拦截并上报 |

## 关键权限常量

| 权限 | 用途 | 影响范围 |
|------|------|---------|
| `ohos.permission.DISTRIBUTED_DATASYNC` | 分布式数据同步 | 设备间数据同步 |
| `ohos.permission.CLOUDDATA_CONFIG` | 云数据配置 | 云同步开关 |
| `ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS` | 跨用户数据访问 | DataShare 跨用户 |
| `ohos.permission.MANAGE_DYNAMIC_UTD_TYPE` | 动态类型管理 | UTD 注册 |

## 数据隔离体系

### 三元组隔离

数据隔离基于 **(用户, 应用, 数据库)** 三元组，由 `StoreMetaData` 管理：

```
元数据 Key: KvStoreMetaData + / + {deviceId} + / + {user} + / + {appId} + / + {storeId}
```

### 隔离层次

| 层次 | 字段 | 机制 |
|------|------|------|
| 用户隔离 | `user` | 不同 OS 用户数据完全隔离，用户删除时连接立即关闭 |
| 应用隔离 | `bundleName` + `appId` | 不同应用数据隔离 |
| 数据库隔离 | `storeId` | 同一应用下不同数据库隔离 |
| 设备隔离 | `deviceId` | 不同设备的数据库副本隔离 |
| 实例隔离 | `instanceId` | 克隆应用使用 `+clone-{id}+` 路径前缀隔离 |
| 区域隔离 | `area` (EL0-EL5) | 数据存储的安全区域路径不同 |

### 路径隔离

`DirectoryManager` 生成的路径结构：

```
/{security}/{store}/{type}/{area}/{userId}/{bundleName}/{hapName}/{storeId}

示例: /misc_de/kv/app/el1/100/com.example.app/entry/mystore
```

关键路径规则：
- 低安全级 (S0/S1) → `misc_de`（设备加密）
- 高安全级 (S2+) → `misc_ce`（凭据加密）
- SA 进程且 user=0 → `/public` 路径
- 克隆应用 → `+clone-{instanceId}+{bundleName}`

## 权限检查流程

### DataShare 权限检查（四层防御）

```
操作请求 (Query/Insert/Update/Delete)
  │
  ├─ 层1: Silent Proxy 门控
  │     检查 calledTokenId 对应 URI 是否允许静默访问
  │     Profile 配置 isSilentProxyEnable=false 时拒绝
  │
  ├─ 层2: ProviderInfo 解析
  │     解析 URI 获取 bundleName、moduleName、storeName、tableName
  │     确定 readPermission / writePermission
  │
  ├─ 层3: 跨账户权限检查
  │     currentUserId == 0 (SA) 或 == visitedUserId → 放行
  │     否则验证 INTERACT_ACROSS_LOCAL_ACCOUNTS 权限
  │     PC 平台禁止跨用户访问
  │
  ├─ 层4: 白名单检查
  │     allowLists 非空时严格匹配 appIdentifier
  │     支持 onlyMain 限制（仅主应用，拒绝克隆应用）
  │
  └─ 层5: 读写权限检查
        读操作验证 readPermission，写操作验证 writePermission
        ProxyData: 禁止空权限（自身 bundleName 访问自身除外）
        Extension: 空权限 = 允许访问
        底层调用 AccessTokenKit::VerifyAccessToken()
```

### 分布式同步权限检查

```
同步请求
  │
  ├─ SyncActivate: 验证用户是否在本地活跃用户列表中
  │     instanceId != 0 禁止同步
  │
  ├─ VerifyPermission:
  │     加载 StoreMetaData → 验证策略范围 → 验证 DISTRIBUTED_DATASYNC
  │
  └─ IsTransferAllowed:
        检查 OS 账户约束 constraint.distributed.transmission.outgoing
        检查双端同步访问限制
```

## 加密体系

### 算法与密钥

| 项目 | 值 |
|------|---|
| 算法 | AES-256-GCM |
| 根密钥 | `"distributed_db_root_key"`，由 HUKS 管理 |
| AAD | `"distributeddata"` |
| NONCE | 12 字节随机数，由 HksGenerateRandom 生成 |

### 安全区域到存储级别映射

| 区域 | HKS 存储级别 |
|------|-------------|
| EL0-EL1 | `HKS_AUTH_STORAGE_LEVEL_DE`（设备加密） |
| EL2-EL3 | `HKS_AUTH_STORAGE_LEVEL_CE`（凭据加密） |
| EL4-EL5 | `HKS_AUTH_STORAGE_LEVEL_ECE`（增强凭据加密） |

CE/ECE 级别密钥操作需要指定 `userId`，用户解锁前不可访问。安全区域不可降级（EL2+ 不可使用 DE 级别 HKS 存储）。

### 密钥管理流程

1. `PrepareRootKey()`：DE 级别直接可用；CE/ECE 级别需先检查是否存在
2. `Encrypt()`：使用根密钥加密数据库密码，生成 nonce 和密文
3. `Decrypt()`：使用根密钥 + nonce 解密
4. `UpdateSecretMeta()`：将加密后的密钥、nonce、时间戳存入 `SecretKeyMetaData`

## 用户切换影响

| 账户状态 | 数据处理 |
|---------|---------|
| `DEVICE_ACCOUNT_DELETE` | 关闭不在活跃用户列表中的数据库连接 |
| `DEVICE_ACCOUNT_STOPPING` | 立即关闭指定用户的数据库连接 |
| `DEVICE_ACCOUNT_STOPPED` | 关闭所有非法用户连接 |
| `DEVICE_ACCOUNT_SWITCHED` | 重新加载 AutoLaunch 元数据 |
| `DEVICE_ACCOUNT_UNLOCKED` | CE/ECE 级别数据可访问 |

## Silent Proxy 安全约束

- 系统应用和白名单应用可使用 Extension 直通模式
- 非系统应用、非白名单应用**强制**走 Silent Proxy 模式
- Profile 配置 `isSilentProxyEnable = false` 可全局关闭 Silent Proxy
- 所有 CRUD 操作前强制检查 Silent Proxy 状态

## Checker 信任验证

`CheckerManager` 采用插件化架构：

- `SetTrustInfo` / `SetDistrustInfo`：配置信任规则
- `IsValid()`：验证 StoreInfo 是否合法
- `IsDistrust()`：验证是否在不信任列表中
- `IsDynamic()` / `IsStatic()`：区分动态/静态存储
- `AppAccessCheckConfigManager`：维护 `appId → bundleName` 的信任映射

## 变更前必读

修改以下内容前必须获取明确批准：

- 权限名称或权限检查逻辑
- 加密算法或密钥管理流程
- 数据隔离字段或路径生成规则
- Silent Proxy 开关逻辑
- 白名单匹配逻辑
- 跨账户访问检查逻辑
- 用户切换时的数据清理逻辑
- `constraint.distributed.transmission.outgoing` 约束检查
