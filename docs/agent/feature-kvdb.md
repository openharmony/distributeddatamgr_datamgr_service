# feature-kvdb.md — KVDB 模块领域知识

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。
> Agent 能自己探索出来的少写；Agent 猜不准、猜错代价高、团队必须统一执行的内容要写。

## 1. Code map

本文件适用于 `service/kvdb/` 及其直接依赖的框架层接口（`AutoCache`、`GeneralStore`、`DeviceMatrix`）。当前无更细粒度子目录 AGENTS.md。

本模块实现 KVDB 功能的服务端协调层，核心职责是端端键值数据同步调度、Store 元数据管理、数据变更通知分发、访问控制。不直接操作数据库文件。

最重要的架构边界：**服务端管理协调，客户端实际存储**。Store 句柄通过 `AutoCache` 获取（`KVDBServiceImpl::Factory::Factory()` 注册 `SINGLE_VERSION` / `DEVICE_COLLABORATION` creator），使用后尽快释放，不得长期持有。

Key areas:
- `service/kvdb/kvdb_service_impl.cpp`: 服务主入口，所有功能编排中心；通过 `FeatureSystem` 注册为 "kv_store" feature（`__attribute__((used))` 静态自注册）
- `service/kvdb/kvdb_general_store.cpp`: `GeneralStore` 的 KVDB 实现，包装 `KvStoreNbDelegate`；`Sync()` 方法分发端端/订阅/取消订阅
- `service/kvdb/kvstore_sync_manager.h`: 单例同步调度器，所有端端同步必须经过 `AddSyncOperation`
- `service/kvdb/kvdb_service_stub.cpp`: IPC Stub，27 个 handler；`CheckPermission()` 是权限校验入口
- `service/kvdb/auth_delegate.h`: 设备访问控制；`AUTH_GROUP_TYPE` 枚举
- `service/matrix/`: `DeviceMatrix` 同步协调，mask/level/broadcast

Where to look:
- 同步行为变更 -> `kvdb_service_impl.cpp` + `kvstore_sync_manager.h`
- 冲突策略变更 -> `kvdb_general_store.cpp` `GetDBOption()`
- IPC 接口码变更 -> `kvdb_service_stub.cpp` + `CODEOWNERS`
- Store 句柄生命周期变更 -> `auto_cache.h` + `auto_cache.cpp` `Delegate::~Delegate()`
- 权限/访问控制变更 -> `auth_delegate.h` + `kvdb_service_stub.cpp` `CheckPermission()`

## 2. Knowledge routing

Before planning or editing, classify the task and read the matching documents.

### Task-based routing
- 同步行为变更 -> 读 `kvdb_service_impl.cpp`（Sync→DoSyncInOrder→DoSyncBegin→DoComplete 流程）+ `kvstore_sync_manager.h`
- IPC 接口码变更 -> 读 `kvdb_service_stub.cpp` + `CODEOWNERS`（必须通知指定评审人）
- 错误码变更 -> 读 `general_error.h` + 本文件 §3 错误码边界
- 冲突策略变更 -> 读 `kvdb_general_store.cpp` `GetDBOption()` + 本文件 §3 P3
- Store 句柄/生命周期变更 -> 读 `auto_cache.h` + `auto_cache.cpp` `Delegate::~Delegate()` + 本文件 §3 P2
- 元数据同步变更 -> 读 `device_matrix.h` + `kvdb_service_impl.cpp` IsNeedMetaSync
- 权限/访问控制变更 -> 读 `auth_delegate.h` + `kvdb_service_stub.cpp` `CheckPermission()`

### Path-based routing
- `service/kvdb/` -> 读本文件 + `kvstore_sync_manager.h`
- `service/matrix/` -> 读 `device_matrix.h`
- `framework/include/store/` -> 读 `auto_cache.h` + `general_store.h`

### Vocabulary-based routing

When the task, issue, log, API, or changed file mentions these terms, read the linked document before planning:

| Term | Risk hint | Read |
|---|---|---|
| AutoCache | Delegate 析构无条件 Close Store；句柄不得长期持有 | `auto_cache.h` + `auto_cache.cpp` `Delegate::~Delegate()` |
| KvStoreSyncManager | 所有端端同步必须经过 AddSyncOperation，不得绕过 | `kvstore_sync_manager.h` |
| DeviceMatrix | mask/level 同步协调；CURRENT_VERSION=3 | `device_matrix.h` |
| conflictResolvePolicy | SINGLE_VERSION→LAST_WIN, DEVICE_COLLABORATION→DEVICE_COLLABORATION，混淆会静默覆盖数据 | `kvdb_general_store.cpp` `GetDBOption()` |
| SyncMode | NEARBY_PUSH/PULL/PULL_PUSH(端端) / SUBSCRIBE_REMOTE/UNSUBSCRIBE_REMOTE(订阅)；SUBSCRIBE_REMOTE 不是端端同步 | `general_store.h` SyncMode enum |
| IPC_SEND | do-while 宏（非 lambda）；传入已 move 对象会导致 Marshal 序列化空值 | `kvdb_notifier_proxy.cpp` IPC_SEND 宏定义 |
| RADAR_REPORT | HiSysEvent 雷达上报宏，不可删除 | `kv_radar_reporter.h` |
| KVDBServiceInterfaceCode | 27 个 IPC 接口码（TRANS_HEAD 到 TRANS_DELETE_BY_OPTIONS），变更需 CODEOWNERS 评审 | `kvdb_service_stub.cpp` HANDLERS 数组 |
| Status / GeneralError / DBStatus | 三层错误码不可混用，必须经转换函数 | `general_error.h` + `kvdb_general_store.h` `ConvertStatus()` + `kvdb_service_impl.h` `ConvertGeneralErr()` + `ConvertDbStatus()` |
| FeatureSystem | KVDB 注册为 "kv_store" feature，`__attribute__((used))` 静态自注册 | `kvdb_service_impl.cpp` `Factory::Factory()` |
| SyncAgent | KVDBServiceImpl 内部结构体，跟踪每个 tokenId 的 pid/notifier/delayTimes/watchers | `kvdb_service_impl.h` SyncAgent struct |
| AUTH_GROUP_TYPE | IDENTICAL_ACCOUNT_GROUP=1, PEER_TO_PEER_GROUP=256, ACROSS_ACCOUNT_AUTHORIZE_GROUP=1282 | `auth_delegate.h` AUTH_GROUP_TYPE enum |

In the plan, state:
- task category
- documents read
- constraints found
- whether a skill/workflow should be used

## 3. Constraints and boundaries

### Architecture/domain invariants
- 服务端管理协调，客户端实际存储；Store 句柄通过 AutoCache 获取，不得长期持有作为成员变量
- 所有端端同步必须经过 `KvStoreSyncManager::AddSyncOperation`，不得绕过（见 `KVDBServiceImpl::Sync()` 实现）
- 功能模块之间不得直接依赖，需通过框架层接口或 EventCenter 交互
- IPC 接口码变更必须通知 CODEOWNERS 指定评审人
- 冲突策略由 storeType 和 isPublic 决定，不得随意变更默认值

### 冲突策略表

由 `StoreMetaData.storeType` 和 `StoreMetaDataLocal.isPublic` 决定，在 `KVDBGeneralStore::GetDBOption()` 中设置：

| storeType | isPublic | conflictResolvePolicy | 代码依据 |
|---|---|---|---|
| SINGLE_VERSION | false | LAST_WIN | `else if (data.storeType == KvStoreType::SINGLE_VERSION)` |
| SINGLE_VERSION | true | DEVICE_COLLABORATION | `local.isPublic` 条件优先 |
| DEVICE_COLLABORATION | 任意 | DEVICE_COLLABORATION | `data.storeType == KvStoreType::DEVICE_COLLABORATION` |
| 内部元数据 (appId==Bootstrap::GetProcessLabel()) | 任意 | LAST_WIN | `data.appId == Bootstrap::GetInstance().GetProcessLabel()` 覆盖 |

### 错误码转换边界（三层，不可混用）

| 层 | 类型 | 来源文件 | 用途 |
|---|---|---|---|
| 服务端统一 | `GeneralError` | `framework/include/error/general_error.h` | 服务内部通用 |
| DB 引擎 | `DBStatus` | DistributedDB `db_errno.h`（外部依赖） | 存储引擎内部 |
| 公共 API | `Status` | 外部依赖 kv_store `store_errno.h` | IPC 传输 |

转换函数（均在 `kvdb_service_impl.cpp` 和 `kvdb_general_store.cpp` 中）：
- `DBStatus→GeneralError`：`KVDBGeneralStore::ConvertStatus()`（静态映射表 `dbStatusMap_`）
- `GeneralError→Status`：`KVDBServiceImpl::ConvertGeneralErr()`
- `DBStatus→Status`：`KVDBServiceImpl::ConvertDbStatus()`

### Do not
- NEVER 长期持有 AutoCache 获取的 Store 句柄作为成员变量
- NEVER 绕过 KvStoreSyncManager 直接调用 delegate_->Sync 进行端端同步
- NEVER 修改 IPC 接口码而不通知 CODEOWNERS
- NEVER 变更冲突策略默认值除非任务明确要求
- NEVER 变更公共 API 签名/错误码/权限行为除非任务明确要求
- NEVER 为通过测试删除日志/事件/错误码（含 RADAR_REPORT）
- NEVER 明文打印 udid/uuid/ip/mac/密钥/路径 — 用 `Anonymous()` 匿名化
- NEVER 传入已 move 对象给 IPC_SEND（Marshal 会序列化空值）
- NEVER 变更序列化格式或持久化数据结构除非任务明确要求并提供迁移逻辑

### Known Pitfalls

**P1: SUBSCRIBE_REMOTE 不是端端同步** — `GeneralStore::SyncMode` 枚举值按区间分发（`general_store.h`）：值 0-2（NEARBY_PUSH/PULL/PULL_PUSH）→ 端端同步 `delegate_->Sync()`；值 10（NEARBY_SUBSCRIBE_REMOTE）→ 订阅远端变更 `delegate_->SubscribeRemoteQuery()`；值 11（NEARBY_UNSUBSCRIBE_REMOTE）→ 取消订阅 `delegate_->UnSubscribeRemoteQuery()`。Agent 误把 SUBSCRIBE_REMOTE 当作端端同步，实际它走 Subscribe 而非 Sync。

**P2: AutoCache Delegate 析构无条件 Close Store** — `Delegate::~Delegate()` 无条件调用 `store_->Close(true)` + `store_->Release()`，不检查外部是否还持有 shared_ptr。AutoCache 垃圾回收触发 Delegate 析构时，Store 会被强制关闭。

**P3: DEVICE_COLLABORATION 冲突策略不是 LAST_WIN** — SINGLE_VERSION 用 LAST_WIN（后写覆盖），DEVICE_COLLABORATION 用 DEVICE_COLLABORATION（保留所有设备数据）。混淆会导致多设备数据被静默覆盖。来源：`KVDBGeneralStore::GetDBOption()` 中 `isPublic || DEVICE_COLLABORATION → DEVICE_COLLABORATION`，`SINGLE_VERSION → LAST_WIN`。

**P4: IKVDBNotifier 回调全部 TF_ASYNC** — `KVDBNotifierProxy` 中 IPC_SEND 宏使用 `MessageOption::TF_ASYNC`，回调不保证送达，不得假设客户端已收到。

**P5: KVDBServiceImpl::Factory 静态注册** — `__attribute__((used))` 进程加载时自动注册 "kv_store" feature 和 AutoCache creator，不需要手动初始化。

**P6: 开关数据 IPC 权限分流** — codes TRANS_PUT_SWITCH 到 TRANS_UNSUBSCRIBE_SWITCH_DATA 用 `CheckerManager::IsSwitches()`，其它 codes 用 `CheckerManager::IsValid()`（见 `KVDBServiceStub::CheckPermission()`）。Agent 误对所有 codes 统一用 IsValid。

### Ask before
- 新增生产依赖
- 变更公共 API 语义
- 变更权限模型或信任边界
- 变更协议兼容性或持久化数据格式
- 删除兼容性适配或迁移逻辑
- 变更功能模块编译开关（`datamgr_service_kvdb`）
- 变更 IPC 接口码
- 执行可能影响连接设备的操作

## 4. Verification

### Minimum checks
- Build KVDB 模块：`./build.sh --product-name <product> --build-target distributeddata_kvdb`
- Build 全量服务：`./build.sh --product-name <product> --build-target datamgr_service`
- Test：`./build.sh --product-name <product> --build-target datamgr_service_test`
- Lint/static check: 无独立工具；编译期 `-Werror=vla` + sanitize + `-fvisibility=hidden` 集成在 `service/kvdb/BUILD.gn`
- Compatibility check: API 变更时比对 `general_error.h` 错误码兼容性

### Task-specific checks
- 同步行为变更 -> 构建 + `KvdbServiceImplTest` + `KVDBGeneralStoreTest`
- IPC 接口码变更 -> 构建 + `KvdbServiceImplTest` + 通知 CODEOWNERS
- 冲突策略变更 -> 构建 + `KVDBGeneralStoreTest`
- 权限/访问控制变更 -> 构建 + `AuthDelegateMockTest`
- DFX/日志变更 -> 构建 + 跑附近单测 + 不可删除 RADAR_REPORT
- 仅测试变更 -> 跑变更的测试 + 至少一个附近相关测试

### Done definition
A task is done only when:
- 请求的行为已实现
- 相关构建/测试/lint/兼容性检查已执行，或已说明无法执行的原因
- 最终回复包含变更摘要、变更文件、验证结果、剩余风险
- 不包含无关的格式化、重构或附带变更

### Final response format
When finishing a non-trivial task, include:
- 变更摘要
- 变更文件列表
- 验证命令与结果
- 兼容性、权限、DFX 影响（如相关）
- 剩余风险或后续事项