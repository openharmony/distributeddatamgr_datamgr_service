# feature-rdb.md — RDB 模块领域知识

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。
> Agent 能自己探索出来的少写；Agent 猜不准、猜错代价高、团队必须统一执行的内容要写。

## 1. 代码地图

本文件适用于 `services/distributeddataservice/service/rdb/` 及其直接依赖的框架层接口（`GeneralStore`、`CloudConflictHandler`、`CloudMark`、`CloudDB`、`SchemaMeta`、`CloudLastSyncInfo`）。当前无更细粒度子目录 AGENTS.md。

本模块实现 RDB（关系型数据库）功能的服务端协调层，核心职责是端端/端云关系型数据同步调度、冲突处理、水印管理、自动同步触发、访问控制、流控。不直接操作数据库文件——实际存储由客户端通过 DistributedDB `RelationalStoreDelegate` 完成。

最重要的架构边界：**服务端管理协调，客户端实际存储**。Store 句柄通过 `AutoCache` 获取（`RdbServiceImpl::Factory::Factory()` 注册 creator），使用后尽快释放，不得长期持有。端云同步经过 `SyncManager`（见 `docs/agent/feature-cloud.md`），端端同步经过 `RdbGeneralStore::Sync` 直接下发 DistributedDB。

关键区域：
- `service/rdb/rdb_service_impl.cpp`: 服务主入口，所有功能编排中心；通过 `FeatureSystem` 注册为 "rdb_service" feature（`__attribute__((used))` 静态自注册）；`Sync()` 按区间分发 `DoSync`（端端）/`DoCloudSync`（端云）
- `service/rdb/rdb_general_store.cpp`: `GeneralStore` 的 RDB 实现，包装 `RelationalStoreDelegate`；`Sync()` 按 `SyncMode` 区间分发端端/端云，`SetCloudReference` 处理水印清除，`SetCloudConflictHandler` 安装冲突处理器
- `service/rdb/rdb_service_stub.cpp`: IPC Stub，`HANDLERS[]` 数组映射 `RdbServiceInterfaceCode`；`OnRemoteDoSync`/`OnRemoteDoAsync`/`OnRemoteStopCloudSync` 三类入口
- `service/rdb/rdb_cloud.cpp`: `RdbCloud : ICloudDb` 云端适配器，桥接 Rust 扩展 `CloudDB`；`BatchInsert/Update/Delete/Query/Lock/HeartBeat/HasCloudUpdate/StopCloudSync`
- `service/rdb/rdb_cloud_conflict_handler.cpp`: `RdbCloudConflictHandler : ICloudConflictHandler`，将框架层 int 协议码映射到 `DistributedDB::ConflictRet`
- `service/rdb/rdb_query.cpp`: `RdbQuery : GenQuery`，predicates→`DistributedDB::Query` 适配器
- `service/rdb/rdb_notifier_proxy.cpp`: Notifier IPC 代理，异步同步结果下发（`TF_ASYNC`，不保证送达）
- `service/rdb/rdb_common_utils.cpp`: `RdbCommonUtils`，`ConvertNativeRdbStatus`（`DBStatus`↔`SyncResultCode`）转换

查找位置：
- 端端同步行为变更 -> `rdb_service_impl.cpp`（`Sync`→`DoSync`→`GetSyncTask`）+ `rdb_general_store.cpp` `Sync`/`GetDBBriefCB`
- 端云同步行为变更 -> `rdb_service_impl.cpp` `DoCloudSync` + `rdb_general_store.cpp` `DoCloudSync`/`GetDBProcessCB` + `docs/agent/feature-cloud.md`
- 冲突策略变更 -> `rdb_cloud_conflict_handler.cpp` + `rdb_general_store.cpp` `SetCloudConflictHandler`/`ConvertPolicy`/`UpdateCloudConfig`
- 水印机制变更 -> `rdb_general_store.cpp` `SetCloudReference` + `framework/include/cloud/cloud_mark.h`
- 自动同步触发变更 -> `rdb_service_impl.cpp` `OnReady`/`DoAutoSync`/`SaveAutoSyncInfo`
- IPC 接口码变更 -> `rdb_service_stub.cpp` `HANDLERS[]` + `CODEOWNERS`
- 流控变更 -> `rdb_service_impl.cpp` `IsSyncLimitApp`/`rdbFlowControlManager_`

## 2. 知识路由

规划或编辑前，先对任务分类并阅读匹配文档。

### 按任务路由
- 端端同步行为变更 -> 读 `rdb_service_impl.cpp`（`Sync`→`DoSync`→`GetSyncTask`→`store->Sync`）+ `rdb_general_store.cpp` `Sync`/`GetDBBriefCB` + 本文件 §3.1
- 端云同步行为变更 -> 读 `docs/agent/feature-cloud.md`（`SyncManager`/`GeneralStore`/`CloudServer`）+ `rdb_service_impl.cpp` `DoCloudSync` + `rdb_general_store.cpp` `DoCloudSync`/`GetDBProcessCB`
- 冲突处理变更 -> 读 `rdb_cloud_conflict_handler.cpp` + `rdb_general_store.cpp` `SetCloudConflictHandler`/`ConvertPolicy`
- 水印/Schema 升级变更 -> 读 `rdb_general_store.cpp` `SetCloudReference` + `cloud_mark.h` + 本文件 §3.4
- IPC 接口码变更 -> 读 `rdb_service_stub.cpp` `HANDLERS[]` + `CODEOWNERS`
- 流控变更 -> 读 `rdb_service_impl.cpp` `IsSyncLimitApp`/`rdbFlowControlManager_` + 本文件 §3.5
- 自动同步触发变更 -> 读 `rdb_service_impl.cpp` `OnReady`/`DoAutoSync` + `schema_meta.h` `AutoSyncType`
- 错误码变更 -> 读 `rdb_common_utils.cpp` `ConvertNativeRdbStatus` + 本文件 §3.2

### 按路径路由
- `service/rdb/` -> 读本文件 + 相关源文件
- `framework/include/store/general_store.h` -> 读 `SyncMode`/`HighMode`/`CleanMode`/`ConflictResolution` enum + 本文件 §3.1
- `framework/include/cloud/` -> 读 `docs/agent/feature-cloud.md`（跨模块协议）

### 按词汇路由

当任务、issue、日志、API 或变更文件出现以下术语时，规划前先阅读链接文档：

| 术语 | 风险提示 | 阅读 |
|---|---|---|
| AutoCache | Delegate 析构无条件 Close Store；句柄不得长期持有 | `auto_cache.h` + `rdb_service_impl.cpp` `GetStore` |
| RdbServiceImpl | RDB 编排中心，"rdb_service" feature；`Sync` 按区间分发端端/端云 | `rdb_service_impl.cpp` |
| RdbGeneralStore | `GeneralStore` RDB 实现；`Sync` 按 `SyncMode` 区间分发 | `rdb_general_store.cpp` |
| RdbCloud | `ICloudDb` 适配器，桥接 Rust `CloudDB`；端云路径必经 | `rdb_cloud.cpp` + `docs/agent/feature-cloud.md` |
| RdbCloudConflictHandler | int 协议码（INSERT/UPDATE/DELETE/NOT_HANDLE/INTEGRATE）→`ConflictRet` 映射 | `rdb_cloud_conflict_handler.cpp` |
| CloudMark | `isClearWaterMark` 元数据标志，Schema 版本变更时设置；触发 `ClearMetaData(CLOUD_WATERMARK)` | `cloud_mark.h` + 本文件 §3.4 |
| SyncMode | NEARBY_PUSH/PULL/PULL_PUSH(端端) / CLOUD_TIME_FIRST/NATIVE_FIRST/CLOUD_FIRST(端云) 按区间分发，混淆会导致走错路径 | `general_store.h` SyncMode enum + 本文件 §3.1 |
| HighMode | MANUAL/AUTO/ASSETS_SYNC_MODE（高16位），`MixMode` OR 组合；ASSETS 仅 CLOUD_CLOUD_FIRST | `general_store.h` HighMode enum |
| AutoSyncType | IS_NOT_AUTO_SYNC/SYNC_ON_CHANGE/SYNC_ON_READY/SYNC_ON_CHANGE_READY，决定设备上线自动同步范围 | `schema_meta.h` AutoSyncType enum |
| HasCloudUpdate | 本地水印探针，实际水印存储在 DistributedDB 内部，本仓库只触发清除和探测 | `cloud_db.h` + 本文件 §3.4 |
| RdbServiceInterfaceCode | IPC 接口码，`HANDLERS[]` 映射，变更需 CODEOWNERS 评审 | `rdb_service_stub.cpp` HANDLERS 数组 |
| SyncParam | 端云同步参数包（mode/wait/isCompensation/isDownloadOnly 等），跨层传递 | `general_value.h` SyncParam struct |
| SyncResultCode | 公共 API 错误码，需经 `ConvertNativeRdbStatus` 转换，不可与 `DBStatus`/`GeneralError` 混用 | `rdb_common_utils.cpp` + 本文件 §3.2 |
| FeatureSystem | RDB 注册为 "rdb_service" feature，`__attribute__((used))` 静态自注册 | `rdb_service_impl.cpp` `Factory::Factory()` |

在计划中声明：
- 任务类别
- 已读文档
- 发现的约束
- 是否应使用特定 Skill/工作流

## 3. 约束与边界

### 架构/领域不变量
- 服务端管理协调，客户端实际存储；Store 句柄通过 AutoCache 获取，不得长期持有作为成员变量
- 功能模块之间不得直接依赖；端云同步通过 `EventCenter` 投递 `ChangeEvent` 由 `SyncManager` 接管（见 `docs/agent/feature-cloud.md`）
- 端端同步直接经 `RdbGeneralStore::Sync` 下发 DistributedDB，不经 `SyncManager`
- IPC 接口码变更必须通知 CODEOWNERS 指定评审人
- 同步路径由 `SyncMode` 区间分发，不得跨区间调用

### 3.1 同步模型

`GeneralStore::SyncMode` 按区间分发（`general_store.h`）：NEARBY (0-3) 端端 `delegate_->Sync`；CLOUD (4-9) 端云 `DoCloudSync`（经 `SyncManager`）；订阅 (10-11) `delegate_->Subscribe/Unsubscribe`。详见 P1/P5。

端端同步：`RdbServiceImpl::Sync` → `DoSync` → `GetSyncTask` → `RdbGeneralStore::Sync` → `delegate_->Sync`。
端云同步：`RdbServiceImpl::Sync` → `DoCloudSync` → `EventCenter::PostEvent(ChangeEvent)` → `CloudData::SyncManager` → `RdbGeneralStore::DoCloudSync` → `delegate_->Sync`。
自动同步：`RdbServiceImpl::OnReady` → `DoAutoSync`（按 `AutoSyncType` 过滤）→ `store->Sync`。

### 3.2 错误码转换边界（三层，不可混用）

| 层 | 类型 | 来源 | 用途 |
|---|---|---|---|
| 服务端统一 | `GeneralError` | `framework/include/error/general_error.h` | 服务内部通用 |
| DB 引擎 | `DBStatus` | DistributedDB `db_errno.h`（外部依赖） | 存储引擎内部 |
| 公共 API | `SyncResultCode` | 外部依赖 distributed_rdb `rdb_types.h` | IPC 传输 |

转换函数：`RdbCommonUtils::ConvertNativeRdbStatus`（`rdb_common_utils.cpp`）—— `DBStatus`↔`SyncResultCode` 映射。端云路径另有 `SyncManager` 的错误码协议（见 `docs/agent/feature-cloud.md` `SharingCenter` 错误码）。

### 3.3 冲突处理

冲突处理由 `RdbCloudConflictHandler`（`rdb_cloud_conflict_handler.cpp`）实现，将框架层 int 协议码（`INSERT`/`UPDATE`/`DELETE`/`NOT_HANDLE`/`INTEGRATE`）映射到 `DistributedDB::ConflictRet`；资产冲突经 `ConvertPolicy`（`rdb_general_store.cpp`）映射 `AssetConflictPolicy`；本地插入冲突用 `GeneralStore::ConflictResolution`（`general_store.h`）。详见源文件，不在此赘述。

### 3.4 水印机制

本仓库**不包含**分布式水印存储/更新逻辑——逐记录同步水印由 DistributedDB 云同步引擎内部管理。本仓库只暴露两类水印相关能力：

**CloudMark — 清除水印元数据标志**（`framework/include/cloud/cloud_mark.h`）：
- `CloudMark final : public Serializable`，字段 `bool isClearWaterMark = false;` + `bundleName/deviceId/storeId/userId/index`
- 由 `StoreMetaData` 构造；`GetKey()` 为 KV 元数据键
- **设置**：`CloudServiceImpl::UpdateClearWaterMark`（`service/cloud/cloud_service_impl.cpp`）—— 当 `SchemaMeta.version` 在新旧间变化时，置 `metaData.isClearWaterMark = true`，日志 `"clear watermark, storeId:..., newVersion:..., oldVersion:..."`
- **消费**：`RdbGeneralStore::SetCloudReference`（`rdb_general_store.cpp`）：
  ```
  CloudMark metaData(meta_);
  if (LoadMeta(metaData.GetKey(), metaData, true) && metaData.isClearWaterMark) {
      ClearMetaDataOption option{.mode = ClearMetaDataMode::CLOUD_WATERMARK};
      delegate_->ClearMetaData(option);   // 清除本地云水印
      DelMeta(metaData.GetKey(), true);
      EventCenter::PostEvent(CloudEvent(UPGRADE_SCHEMA, GetStoreInfo()));
  }
  ```
  在 `SetDistributedTables` 调用且 `DistributedTableType::DISTRIBUTED_CLOUD` 时触发（`rdb_general_store.cpp`）。

**HasCloudUpdate — 本地水印探针**（`framework/include/cloud/cloud_db.h`）：
- `CloudDB::HasCloudUpdate(tableName, localWaterMark)`，base 返回 `true`
- `RdbCloud::HasCloudUpdate`（`rdb_cloud.cpp`）转发 `cloudDB_->HasCloudUpdate(...)`
- Rust 扩展 `CloudDbImpl::HasCloudUpdate`（`rust/extension/cloud_db_impl.cpp`）当前返回 `true`
- `localWaterMark` 为 `std::string`，由 DistributedDB 传入，其实际存储/更新不在本仓库

**CleanMode 水印**：`GeneralStore::CleanMode` 含 `CLEAN_WATER`（`general_store.h`），`Clean(...)` 用于清除同步水印数据。

**云游标（Cursor）**：云端逐记录进度令牌，`SchemaMeta::CURSOR_FIELD = "#_cursor"`（`schema_meta.h`），另含 `CURSOR_EXPIRE = "#_cursor_expire"`；由 `CloudCursorImpl`（`rust/extension/cloud_cursor_impl.cpp`）从 Rust 读取游标 blob 写入 `entry[CURSOR_FIELD]`。

### 3.5 流控

`RdbServiceImpl::IsSyncLimitApp(meta)`（`rdb_service_impl.cpp`）判断是否限流：检查 `SyncManager::IsAutoSyncApp` + 数据库 meta；命中则同步任务经 `rdbFlowControlManager_->Execute(task, {0, meta.bundleName})` 调度。

限流参数（`rdb_service_impl.h`）：
- `SYNC_APP_LIMIT_TIMES = 5`（单应用每窗口次数上限）
- `SYNC_GLOBAL_LIMIT_TIMES = 20`（全局每窗口次数上限）
- `SYNC_DURATION = 60 * 1000`（1 分钟窗口）
- `SAVE_CHANNEL_INTERVAL = 5`（分钟，特殊设备通道缓存）
- `WAIT_TIME = 30 * 1000`（端云同步默认等待，ms）
- `SHARE_WAIT_TIME = 60`（秒）

Agent 注意：不要绕过流控管理器直接调用 `store->Sync`；不要变更限流默认值除非任务明确要求。

### 禁止事项
- NEVER 长期持有 AutoCache 获取的 Store 句柄作为成员变量
- NEVER 变更 `SyncMode` 区间语义（端端/端云/订阅的区间划分）
- NEVER 变更冲突处理返回码协议（INSERT/UPDATE/DELETE/NOT_HANDLE/INTEGRATE）的数值
- NEVER 在异步操作回调中捕获 this（UAF 风险，见 AGENTS.md §3）
- NEVER 变更公共 API 签名/错误码/权限行为除非任务明确要求
- NEVER 为通过测试删除日志/事件/错误码
- NEVER 变更 `AssetConflictPolicy` 默认值除非任务明确要求
- NEVER 明文打印 udid/uuid/ip/mac/密钥/路径 — 用 `Anonymous()` 匿名化
- NEVER 变更序列化格式或持久化数据结构除非任务明确要求并提供迁移逻辑
- NEVER 与其它 feature 产生依赖关系 - feature 间通信使用 EventCenter

### 已知陷阱

**P1: SyncMode 区间分发，跨区间调用走错路径** — `GeneralStore::SyncMode` 按区间分发：值 0-2（NEARBY_PUSH/PULL/PULL_PUSH）→ 端端 `delegate_->Sync()`；值 4-8（CLOUD_TIME_FIRST..CLOUD_CUSTOM_PULL）→ `DoCloudSync`（端云，经 `SyncManager`）；值 10-11（SUBSCRIBE/UNSUBSCRIBE_REMOTE）→ 订阅。Agent 误把端云 mode 当端端调用，或反之。

**P2: HasCloudUpdate 不存水印，只是探针** — 本仓库无 `Watermark` 类；逐记录水印存储/更新在 DistributedDB 内部。`HasCloudUpdate(tableName, localWaterMark)` 只是"是否有云更新需要同步"的探测，base/Rust 当前恒返回 `true`。Agent 误在此处找水印存储逻辑。

**P3: CloudMark 是清除标志，不是水印本身** — `CloudMark.isClearWaterMark` 仅在 Schema 版本变更时置 true，触发 `delegate_->ClearMetaData(CLOUD_WATERMARK)` 清除本地云水印后删除该 meta；它不是持续记录同步进度的水印。

**P4: HighMode 与 SyncMode 用 MixMode OR 组合** — `MixMode(syncMode, highMode)` 按位 OR；`GetSyncMode` 取低16位，`GetHighMode` 取高16位。`ASSETS_SYNC_MODE`（0x20000）仅在 `predicates 非空 && CLOUD_CLOUD_FIRST` 时选用，否则 AUTO/MANUAL。Agent 误把 HighMode 当 SyncMode 比较。

**P5: 端端同步不经 SyncManager，端云同步经 SyncManager** — 端端 `RdbGeneralStore::Sync` 直接调 `delegate_->Sync`；端云通过 `EventCenter::PostEvent(ChangeEvent)` 投递给 `CloudData::SyncManager` 接管（重试、进度、`CloudLastSyncInfo` 持久化）。不要在端端路径引入 SyncManager。

**P6: 元数据先行（IsNeedMetaSync）** — `RdbServiceImpl::IsNeedMetaSync`（`rdb_service_impl.cpp`）通过 `CapMetaData` + `DeviceMatrix` mask（`META_STORE_MASK`）判断元数据是否需要先同步；命中则 `MetaDataManager::Sync` 完成回调内才执行 `store->Sync`。跳过会导致数据同步基于过期元数据。

**P7: RdbCloudConflictHandler 返回码是 int 不是 enum** — 框架 `CloudConflictHandler::HandleConflict` 返回 int（0=INSERT..4=INTEGRATE），RDB 适配层再映射到 `DistributedDB::ConflictRet`。新增冲突策略必须使用这套 int 协议码，不得引入新返回值。

**P8: RdbNotifierProxy 全部 TF_ASYNC** — 异步同步结果下发用 `MessageOption::TF_ASYNC`，回调不保证送达，不得假设客户端已收到结果。

**P9: device 标识转换 networkId→uuid** — 调用方传 `networkId`，`ConvertToDeviceInfo`（`rdb_service_impl.cpp`）经 `DmAdapter::ToUUID` 转 uuid；DistributedDB sync 基于 uuid。`ObtainDistributedTableName` 也用 uuid 生成远端设备本地表名。

**P10: RdbServiceImpl::Factory 静态注册** — `__attribute__((used))` 进程加载时自动注册 "rdb_service" feature 和 AutoCache creator，不需手动初始化。

### 询问后再做
- 新增生产依赖
- 变更公共 API 语义
- 变更权限模型或信任边界
- 变更协议兼容性或持久化数据格式（含 `CloudMark`、`CloudLastSyncInfo`、`SchemaMeta` 字段）
- 删除兼容性适配或迁移逻辑
- 变更功能模块编译开关（`datamgr_service_rdb`）
- 变更 IPC 接口码
- 变更限流默认值（`SYNC_APP_LIMIT_TIMES`/`SYNC_GLOBAL_LIMIT_TIMES`/`SYNC_DURATION`）
- 执行可能影响连接设备的操作

## 4. 验证

### 最小检查
- Build RDB 模块：`./build.sh --product-name <product> --build-target distributeddata_rdb`
- Build 全量服务：`./build.sh --product-name <product> --build-target datamgr_service`
- Test：`./build.sh --product-name <product> --build-target datamgr_service_test`
- Lint/static check: 无独立工具；编译期 `-Werror=vla` + sanitize + `-fvisibility=hidden` 集成在 `service/rdb/BUILD.gn`
- Compatibility check: API 变更时比对 `SyncResultCode` / `SyncMode` 区间 / 冲突返回码协议兼容性
- Rust 扩展变更 -> `cargo fmt --check`（对照 `rustfmt.toml`）+ 构建 ylong_cloud_extension

### 任务级检查
- 端端同步行为变更 -> 构建 + `RdbServiceImpl` 相关单测 + `RdbGeneralStoreTest`
- 端云同步行为变更 -> 构建 + `SyncManager` 相关集成测试（见 `docs/agent/feature-cloud.md` §4）
- 冲突策略变更 -> 构建 + `RdbCloudConflictHandler` 单测 + `RdbGeneralStoreTest`
- 水印/Schema 升级变更 -> 构建 + `CloudMark`/`SetCloudReference` 单测 + 升级集成测试
- IPC 接口码变更 -> 构建 + `RdbServiceStub` 单测 + 通知 CODEOWNERS
- 流控变更 -> 构建 + 流控单测（`SYNC_APP_LIMIT_TIMES`/`SYNC_GLOBAL_LIMIT_TIMES` 边界）
- DFX/日志变更 -> 构建 + 跑附近单测 + 不可删除 RADAR/OnSyncStart/OnSyncFinish
- Rust 扩展变更 -> 构建 ylong_cloud_extension + 对应单测
- 仅测试变更 -> 跑变更的测试 + 至少一个相邻相关测试

### 完成定义
A task is done only when:
- 请求的行为已实现
- 相关构建/测试/lint/兼容性检查已执行，或已说明无法执行的原因
- 最终回复包含变更摘要、变更文件、验证结果、剩余风险
- 不包含无关的格式化、重构或附带变更
- 测试覆盖：修改和新代码有 UT 覆盖，新增外部接口有 FUZZ 测试覆盖

### 最终回复格式
When finishing a non-trivial task, include:
- 变更摘要
- 变更文件列表
- 验证命令与结果
- 兼容性、权限、DFX 影响（如相关）
- 剩余风险或后续事项

## 5. 关键文件索引

| 文件 | 职责 |
|---|---|
| `services/distributeddataservice/service/rdb/rdb_service_stub.h/.cpp` | IPC Stub，`HANDLERS[]` 分发，`OnRemoteDoSync/DoAsync/StopCloudSync` |
| `services/distributeddataservice/service/rdb/rdb_service_impl.h/.cpp` | `RdbServiceImpl` 编排：`Sync/DoSync/DoCloudSync/DoAutoSync/GetSyncTask/IsNeedMetaSync/ProcessResult/OnReady` |
| `services/distributeddataservice/service/rdb/rdb_general_store.h/.cpp` | `RdbGeneralStore`（`GeneralStore` 实现）：`Sync/DoCloudSync/GetDBProcessCB/GetDBBriefCB/OnSyncStart/OnSyncFinish/SetCloudReference/SetCloudConflictHandler/ConvertPolicy/UpdateCloudConfig` |
| `services/distributeddataservice/service/rdb/rdb_cloud.h/.cpp` | `RdbCloud : ICloudDb` 云端适配器：`BatchInsert/Update/Delete/Query/Lock/HeartBeat/HasCloudUpdate/StopCloudSync` |
| `services/distributeddataservice/service/rdb/rdb_cloud_conflict_handler.h/.cpp` | `RdbCloudConflictHandler : ICloudConflictHandler`，int 协议码→`ConflictRet` 映射 |
| `services/distributeddataservice/service/rdb/rdb_common_utils.h/.cpp` | `RdbCommonUtils`：`ConvertNativeRdbStatus`（`DBStatus`↔`SyncResultCode`） |
| `services/distributeddataservice/service/rdb/rdb_query.h/.cpp` | `RdbQuery : GenQuery`：predicates→`DistributedDB::Query` 适配 |
| `services/distributeddataservice/service/rdb/rdb_notifier_proxy.h/.cpp` | Notifier IPC 代理（异步结果下发，`TF_ASYNC`） |
| `services/distributeddataservice/service/rdb/rdb_watcher.h/.cpp` | `RdbWatcher`（数据变更观察桥） |
| `services/distributeddataservice/service/cloud/sync_manager.h/.cpp` | `CloudData::SyncManager`：端云同步编排，`SyncInfo`/重试/`CloudLastSyncInfo`（见 `docs/agent/feature-cloud.md`） |
| `services/distributeddataservice/framework/include/store/general_store.h` | `GeneralStore` 抽象 + `SyncMode`/`HighMode`/`CleanMode`/`ConflictResolution` enum，`Sync` 虚函数 |
| `services/distributeddataservice/framework/include/store/general_value.h` | `SyncParam`/`SyncStage`/`ReportParam`/`SubscribeCur`/`Asset`/`Reference` |
| `services/distributeddataservice/framework/include/cloud/cloud_conflict_handler.h` | `CloudConflictHandler` 抽象（`HandleConflict` 返回 int） |
| `services/distributeddataservice/framework/include/cloud/cloud_mark.h` | `CloudMark`（`isClearWaterMark` 元数据） |
| `services/distributeddataservice/framework/include/cloud/schema_meta.h` | `SchemaMeta`/`Database`，`AutoSyncType`，`GetSyncTables/GetCloudTables`，云记录字段常量 |
| `services/distributeddataservice/framework/include/cloud/cloud_last_sync_info.h` | `CloudLastSyncInfo`（start/finish/code） |
| `services/distributeddataservice/framework/include/cloud/cloud_db.h` | `CloudDB::HasCloudUpdate(tableName, localWaterMark)` |
| `services/distributeddataservice/framework/cloud/cloud_db.cpp` / `cloud_mark.cpp` | base 实现 |
| `services/distributeddataservice/rust/extension/cloud_db_impl.h/.cpp` | Rust `CloudDbImpl::HasCloudUpdate`（当前返回 true） |
| `services/distributeddataservice/rust/extension/cloud_cursor_impl.h/.cpp` | `CloudCursorImpl` — 云游标（`#_cursor`）处理 |

### 注意事项 / 外部依赖
- `RdbSyncerParam`、`RdbService::Option`（含字段 `mode`/`isAsync`/`isAutoSync`/`isCompensation`/`isDownloadOnly`/`isEnablePredicate`/`isFullSync`/`enableErrorDetail`/`seqNum`/`assetConflictPolicy`）、`SubscribeOption`、`PredicatesMemo`、`AsyncDetail`、`SyncResultCode`、`DistributedRdb::AssetConflictPolicy` 定义在**外部** distributed_rdb 框架头文件（`rdb_service.h`/`rdb_types.h`），本仓库通过 `#include` 消费但不持有源码。
- `DistributedDB::RelationalStoreDelegate`、`DistributedDB::SyncMode`、`DistributedDB::ConflictRet`、`DistributedDB::AssetConflictPolicy`、`DistributedDB::ClearMetaDataMode::CLOUD_WATERMARK` 属于 DistributedDB SDK，不在本仓库。
- 逐记录云同步水印存储/更新逻辑在 DistributedDB 云同步引擎内部；本仓库仅触发清除（`CloudMark.isClearWaterMark` → `delegate_->ClearMetaData(CLOUD_WATERMARK)`）和探测（`HasCloudUpdate(tableName, localWaterMark)`）。
- `RdbServiceInterfaceCode` 枚举值与 `HANDLERS[]` 映射变更需 CODEOWNERS 评审。
