# 数据共享领域知识

## 适用范围

本文档描述跨应用数据静默共享（DataShare）的领域知识，是数据共享/静默访问变更时的官方参考。

## 核心概念

### Silent Proxy（静默代理）

Silent Proxy 是 DataShare 的免 Extension 直通数据库机制。应用无需启动 DataShareExtension 即可直接读写对端数据库。

- 系统应用和白名单应用可使用 Extension 直通模式
- 非系统应用、非白名单应用**强制**走 Silent Proxy 模式

### DataShare URI

DataShare 使用标准 URI 标识数据：

```
datashare:///{bundleName}/{moduleName}/{storeName}/{tableName}
```

URI 至少需要四段才走完整权限检查流程。不足四段的 URI 默认放行。

## 架构

### 核心类

| 类 | 职责 |
|---|---|
| `DataShareServiceImpl` | IPC 入口，管理 CRUD 操作和订阅 |
| `DataShareSilentConfig` | Silent Proxy 开关管理 |
| `DataShareProfileConfig` | Profile 配置管理 |
| `DataProviderConfig` | URI 解析和 Provider 信息获取 |
| `DataShareSaConfigInfoManager` | SA 配置信息管理 |
| `DataShareSchedulerManager` | 操作调度 |

### 策略模式

操作执行采用策略链模式：

```
LoadConfigCommonStrategy
  → LoadConfigFromDataProxyNodeStrategy
    → PermissionStrategy
```

- `LoadConfigCommonStrategy`：从 URI 解析用户 ID、调用者 TokenId、目标 BundleName
- `PermissionStrategy`：检查权限是否为 `"reject"` 或未授权

## 权限检查流程

所有 CRUD 操作经过统一的 `ExecuteEx()` 入口，包含四层检查：

### 层1：ProviderInfo 解析

```
URI → 解析 bundleName, moduleName, storeName, tableName
  → 从 SA Config / ProxyData / Extension 获取 readPermission / writePermission
  → 确定访问模式 (AccessCrossMode): USER_UNDEFINED, USER_SHARED, USER_SINGLE
```

### 层2：跨账户权限检查

```
currentUserId == 0 (SA 调用) → 放行
currentUserId == visitedUserId → 放行
否则 → 验证 ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS
PC 平台 → 完全禁止跨用户访问
```

### 层3：白名单检查

```
allowLists 为空 → 跳过检查
allowLists 非空:
  SA 调用者 → 按 processName 匹配
  HAP 调用者 → 按 appIdentifier 匹配
  onlyMain=true → 仅允许主应用，拒绝克隆应用
```

### 层4：读写权限检查

```
读操作 → 验证 readPermission
写操作 → 验证 writePermission

Extension 来源:
  空权限 → 允许访问
  有权限 → 验证

ProxyData 来源:
  自身 bundleName 访问自身 → 允许
  空权限 → 拒绝（除自身外）
  "noPermission" → 放行
  其他指定权限 → 使用 AccessTokenKit::VerifyAccessToken() 验证
```

## Silent Proxy 开关管理

### 开启/关闭

```
EnableSilentProxy():
  调用者使用 TokenId 注册 URI 的启用状态
  支持 "*" 通配所有 URI
  空 URI 等价于全部启用

IsSilentProxyEnable():
  第一层: 运行时缓存检查（TokenId + URI 精确匹配）
  第二层: 未命中缓存 → 读取 DataShareProfileConfig
    Profile isSilentProxyEnable=false → 拒绝
    无 Profile 配置 → 默认允许
```

### 安全约束

- `GetSilentProxyStatus()` 在 Helper 创建时还会验证 BMS 和 MetaData 状态
- 所有 CRUD 操作前**强制**检查 Silent Proxy 状态
- Profile 配置 `isSilentProxyEnable = false` 可全局关闭

## 订阅/发布机制

数据变更订阅使用策略链执行：

```
SubscribeStrategy::Execute()
  → LoadConfigCommonStrategy: 解析上下文
    → PermissionStrategy: 检查 context->permission

PublishStrategy::Execute()
  类似流程，增加通知分发
```

## DataShare 约束

- ProxyData 模式禁止空权限（自身访问除外）
- Extension 直通模式仅限系统应用和白名单应用
- 白名单非空时严格匹配 appIdentifier
- PC 平台禁止跨用户访问数据
- `onlyMain` 字段可限制克隆应用访问
- Predicate 需通过 `VerifyPredicates()` 安全检查
- SA 配置变更需要通知所有活跃订阅者
