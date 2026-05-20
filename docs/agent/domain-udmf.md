# UDMF 领域知识

## 适用范围

本文档描述统一数据管理框架（UDMF）和统一类型定义（UTD）的领域知识，是 UDMF/UTD 相关变更时的官方参考。

## UDMF 概述

UDMF（Unified Data Management Framework）是系统级统一数据管理标准，为跨应用数据交互提供标准化框架。支持以下场景：

- 拖拽（Drag）
- 系统分享（System Share）
- 上下文菜单（Menu）
- 文件选择器（Picker）
- 数据中心（Data Hub）

## UDMF 架构

```
service/udmf/
  ├── udmf_service_impl.h/.cpp     # IPC 入口
  ├── udmf_service_stub.h/.cpp     # IPC Stub
  ├── store/                        # 存储层
  │   ├── store.h                   # 抽象 Store 接口
  │   ├── runtime_store.h/.cpp      # KV 存储实现
  │   ├── store_cache.h/.cpp        # 单例缓存 + GC
  │   └── drag_observer.h/.cpp      # 拖拽数据观察者
  ├── lifecycle/                    # 生命周期管理
  │   ├── lifecycle_manager.h       # 数据生命周期管理
  │   ├── lifecycle_policy.h        # 策略：OnGot, OnStart, OnTimeout
  │   └── drag_lifecycle.h          # 拖拽专用生命周期
  ├── permission/                   # 权限管理
  │   ├── checker_manager.h         # 权限检查协调
  │   ├── data_checker.h            # 数据级权限检查
  │   ├── permission_validator.h    # Token 权限验证
  │   └── uri_permission_manager.h  # URI 授权管理
  ├── preprocess/                   # 数据预处理
  │   ├── data_handler.h            # 数据转换处理器
  │   ├── preprocess_utils.h        # 运行时信息、bundle 名称、文件 URI 处理
  │   └── udmf_notifier.h           # 客户端通知
  └── delay_data/                   # 延迟数据
      ├── delay_data_prepare_container.h   # 延迟数据准备（源端）
      ├── delay_data_acquire_container.h   # 延迟数据获取（目标端）
      └── synced_device_container.h        # 已同步设备追踪
```

## 数据流

### SetData（写入统一数据）

```
UdmfServiceImpl::SetData()
  → DFX 上报
  → DLP 检查 (IsDraggable)
  → SaveData()
    → 验证数据和意图
    → PreProcessUtils::FillRuntimeInfo()  # 填充 tokenId, deviceId, 时间戳
    → PreProcessUtils::HandleFileUris()   # 拖拽意图处理文件 URI
    → StoreCache::GetStore(intention)     # 按意图获取/创建 Store
    → store->Put(unifiedData, summary)    # 持久化到 KV
    → DFX 行为上报 (UDMFReport)
```

### GetData（读取统一数据）

```
UdmfServiceImpl::GetData()
  → 检查延迟数据
  → RetrieveData()
    → 从 Store 读取
    → VerifyDataAccessPermission()   # 使用 CheckerManager 验证权限
    → LifeCycleManager::OnGot()      # 处理生命周期（可能删除数据）
    → ProcessUri()                   # 授予 URI 权限
    → TransferToEntriesIfNeed()      # PC/PAD/2in1 设备类型转换
```

### Sync（同步数据）

```
UdmfServiceImpl::Sync()
  → 检查 native token 和同步权限
  → StoreSync()
    → IsNeedMetaSync() → MetaDataManager::Sync()  # 先同步元数据
    → store->Sync()                                # 再同步数据
```

## Intention（意图）映射

| 意图 | 说明 |
|------|------|
| `UD_INTENTION_DRAG` | 拖拽 |
| `UD_INTENTION_SYSTEM_SHARE` | 系统分享 |
| `UD_INTENTION_MENU` | 上下文菜单 |
| `UD_INTENTION_PICKER` | 文件选择器 |
| `UD_INTENTION_DATA_HUB` | 数据中心 |

每个 Intention 对应独立的 Store 实例，由 `StoreCache` 管理生命周期。

## Store Cache

`StoreCache` 是单例，管理 `ConcurrentMap<string, shared_ptr<Store>>`：

| 方法 | 说明 |
|------|------|
| `GetStore(intention)` | 按意图懒创建 Store |
| `CloseStores()` | 用户切换时关闭所有 Store |
| `RemoveStore(intention)` | 移除损坏的 Store |

自动 GC，1 分钟间隔清理不活跃的 Store。

## 生命周期管理

### LifeCycleManager

| 策略方法 | 说明 |
|---------|------|
| `OnStart()` | 数据创建时 |
| `OnGot()` | 数据读取后（可能删除，除非 `readAndKeep`） |
| `OnTimeout()` | 定时清理（1 小时间隔） |

### 拖拽生命周期

`DragLifecycle` 处理拖拽场景的特殊生命周期需求。

## 权限体系

### CheckerManager（插件化权限检查）

| 检查器 | 说明 |
|--------|------|
| `DataChecker` | 数据级权限检查 |
| `PermissionValidator` | Token 权限验证 |
| `URIPermissionManager` | URI 授权管理 |

### 权限检查流程

1. `VerifyDataAccessPermission()` 使用 `CheckerManager` 协调验证
2. DLP 检查：`IsDraggable` 验证数据是否可拖拽
3. 跨设备数据需要 URI 权限授予（`ProcessUri()`）

## 延迟数据

### 概念

延迟数据（Delay Data）是 UDMF 的优化机制，源端不立即传输完整数据，目标端按需获取。

| 组件 | 说明 |
|------|------|
| `DelayDataPrepareContainer` | 源端：准备延迟数据 |
| `DelayDataAcquireContainer` | 目标端：按需获取延迟数据 |
| `SyncedDeviceContainer` | 追踪已同步设备列表 |

## UTD（统一类型定义）

### 架构

```
service/utd/
  ├── utd_service_impl.h/.cpp   # 服务实现
  └── UtdCfgsChecker            # 格式验证器
```

### 核心操作

| 操作 | 说明 | 权限要求 |
|------|------|---------|
| `RegServiceNotifier()` | 注册客户端通知器 | 系统内部 |
| `RegisterTypeDescriptors()` | 注册新类型描述符 | 系统应用 + `MANAGE_DYNAMIC_UTD_TYPE` |
| `UnregisterTypeDescriptors()` | 注销类型描述符 | 系统应用 + `MANAGE_DYNAMIC_UTD_TYPE` |

### 类型注册/注销流程

1. 验证调用者是否为系统应用
2. 验证 `MANAGE_DYNAMIC_UTD_TYPE` 权限
3. `UtdCfgsChecker` 验证格式
4. 注册/注销类型描述符
5. 使用 `DynamicUtdChange()` 回调通知所有已注册的客户端

### 与 UDMF 的关系

UTD 类型使用 `GetUtdIds()` 与 UDMF 数据记录关联，实现类型感知的数据处理和行为上报。

## UDMF/UTD 约束

- UDMF 默认编译开关为关闭（`datamgr_service_udmf = false`）
- UTD 类型注册需要系统应用权限
- DLP 保护的数据不可拖拽
- 数据生命周期策略可能导致读取后删除
- Store Cache GC 可能影响长时间未访问的数据
- 同步操作需要先同步元数据再同步数据
- PC/PAD/2in1 设备需要类型转换（`TransferToEntriesIfNeed`）
