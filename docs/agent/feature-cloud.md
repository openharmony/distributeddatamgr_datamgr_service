# AGENTS.md

## 1. Code map

本 AGENTS.md 适用于 `services/distributeddataservice/service/cloud/` 目录（端云服务层）。根仓库 `AGENTS.md` 适用于仓库整体。

本模块实现 OpenHarmony **端云数据同步服务**，核心职责是本地数据与云端数据的双向同步、云开关管理、云订阅、云数据共享、数据冲突处理和同步策略配置。最重要的架构边界是 **服务层（CloudServiceImpl/SyncManager）调用框架层抽象接口（CloudServer/CloudDB/GeneralStore），不直接依赖云端协议实现**。

关键文件：

| 文件 | 职责 |
|---|---|
| `sync_manager.h/.cpp` | 同步流程统一管理 |
| `cloud_service_impl.h/.cpp` | IPC 服务实现 |
| `cloud_service_stub.h/.cpp` | IPC Stub |
| `cloud_data_translate.h/.cpp` | 服务层-框架层数据转换（Asset ↔ Blob） |
| `cloud_value_util.h/.cpp` | SharingUtil 值转换（Privilege/Participant/Confirmation） |
| `cloud_types_util.h/.cpp` | CloudTypes 类型序列化工具 |
| `cloud_notifier_proxy.h/.cpp` | IPC 通知代理（OnComplete/OnSyncInfoNotify） |
| `sync_config.h/.cpp` | 同步配置管理 |
| `sync_strategies/network_sync_strategy.h/.cpp` | 网络同步策略（继承 SyncStrategy） |

Where to look：

- 同步流程变更 → `sync_manager.h/.cpp`（本文档 §3 + 风险提示）
- IPC 接口变更 → `cloud_service_impl.h/.cpp` + `cloud_service_stub.h/.cpp`
- 同步策略变更 → `sync_strategies/` + `framework/include/cloud/sync_strategy.h`
- 共享功能变更 → `cloud_value_util.h/.cpp` + `framework/include/cloud/sharing_center.h` + `feature-data-share.md`
- 数据转换/序列化变更 → `cloud_data_translate.h/.cpp` + `cloud_types_util.h/.cpp`
- 通知回调变更 → `cloud_notifier_proxy.h/.cpp`
- 配置或权限变更 → `app/distributed_data.cfg`

## 2. Knowledge routing

以下文档不是可选背景阅读。当任务命中对应类别时，必须在规划前阅读匹配文档。

### Task-based routing

- 端云同步行为变更 → 本文档即为知识源，重点阅读 §3 Constraints 和以下术语表
- 数据共享或 SharingCenter 变更 → 阅读 `feature-data-share.md`
- RDB 侧同步行为关联变更 → 确认与 `feature-rdb.md` 中端端同步规则一致
- KVDB 侧同步行为关联变更 → 确认与 `feature-kvdb.md` 中端端同步规则一致
- 跨模块 IPC 类型（cloud_types.h）变更 → 确认与 `distributeddatamgr_relational_store` 侧兼容

### Path-based routing

- `service/cloud/` → 本文档即为知识源
- `framework/include/cloud/` → 本文档即为知识源
- `framework/include/cloud/sharing_center.h` → `feature-data-share.md`

### Vocabulary-based routing

当任务、issue、日志、API 名称或变更文件中出现以下术语时，在规划前阅读链接文档：

| 术语 | 风险提示 | 阅读 |
|---|---|---|
| CloudSync / 端云同步 | 涉及双向数据一致性、冲突处理、水印管理 | 本文档 §3 + 风险提示 |
| CloudInfo | 云用户信息，包含空间配额和应用开关状态 | 本文档 §3 + 风险提示 |
| SchemaMeta | 云端 Schema 元数据，版本变更影响数据兼容性 | 本文档 §3 + 风险提示 |
| SyncStrategy / NetworkSyncStrategy | 同步策略链，影响网络条件判断和同步触发 | 本文档 §3 + 风险提示 |
| SharingCenter / 云共享 | 跨用户数据共享，涉及 Privilege/Participant 权限模型 | `feature-data-share.md` |
| CloudConflictHandler | 冲突解决策略，影响数据一致性 | 本文档 §3 + 风险提示 |
| Subscription / 云订阅 | 云端订阅生命周期管理，有过期和续订逻辑 | 本文档 §3 + 风险提示 |
| AssetLoader | 资产下载管理，涉及大文件和网络状态，注意超时和资源释放 | 本文档 §3 + 风险提示 |
| CloudNotifierProxy | IPC 通知代理，回调语义变更影响应用层 | 本文档 §3 + 风险提示 |
| SyncConfig / CloudDbSyncConfig | 同步配置管理，影响同步行为参数 | 本文档 §3 + 风险提示 |
| CLOUDFILE_SYNC / 云开关 | 权限和功能入口，不得绕过校验 | 本文档 §3 + 风险提示 |

在计划中声明：
- 任务类别
- 已读文档
- 发现的约束
- 是否应使用特定 Skill/工作流

## 3. Constraints and boundaries

### Architecture/domain invariants

- 端云服务层（CloudServiceImpl/SyncManager）通过 CloudServer 抽象接口获取云组件，不直接依赖 Rust 层或云端协议实现。
- 同步流程必须经过 SyncManager 统一管理，不得在 CloudServiceImpl 中直接调用 GeneralStore::Sync。
- 数据转换（CloudDataTranslate/CloudValueUtil/CloudTypesUtil）是服务层与框架层之间的桥梁，类型映射必须双向一致。
- 同步策略通过 SyncStrategy 责任链管理，新增策略必须继承 SyncStrategy 并注册到链中。
- 网络状态变更通过 NetworkRecoveryManager 处理补偿同步，不得自行实现重连逻辑。
- 云开关状态是同步流程的前置条件，所有同步操作必须检查开关状态。
- 冲突处理通过 CloudConflictHandler 统一处理，不得在同步流程中跳过冲突检测。

### Do not

- 不要绕过 CloudServer 抽象直接调用 Rust 层接口。
- 不要变更 IPC 接口编号、参数顺序或回调语义（除非任务明确要求）。
- 不要在 CloudServiceImpl 中直接调用 GeneralStore::Sync，同步必须经过 SyncManager。
- 不要跳过云开关检查直接执行同步操作。
- 不要跳过冲突处理流程。
- 不要在异步回调中捕获 this 指针（使用弱引用或 ExecutorPool 安全回调）。
- 不要长期持有 AutoCache 获取的数据库句柄作为成员变量。
- 不要为通过测试删除日志、事件、错误码或诊断信息。
- 不要直接修改 generated files，修改 source of truth 后重新生成。
- 不要在没有显式批准的情况下引入新的生产依赖。
- 不要修改 SharingCenter 错误码值（SHARING_ERR_OFFSET = 1000 是跨模块协议）。
- 不要变更 CloudConfig 默认值（maxNumber=30, maxSize=1.5MB, maxRetryConflictTimes=3）除非明确要求。
- 不要变更 SchemaMeta 版本号（除非任务明确要求并提供数据迁移逻辑）。
- 不要在 AssetLoader 下载大文件时忽略超时和资源释放，必须设置合理的超时和清理机制。

### Ask before

- 添加新的第三方依赖。
- 变更 IPC 接口语义或 CloudServiceStub Handler 编号。
- 变更云开关或权限模型。
- 变更同步协议兼容性或 SchemaMeta 版本。
- 变更 SharingCenter 错误码或 Privilege/Participant 类型定义。
- 删除兼容性适配或迁移逻辑。
- 变更 CloudConfig 默认参数。
- 执行可能影响连接设备或真实云端数据的操作。

## 4. Verification

### Minimum checks

- 构建当前模块：`./build.sh --product-name <product> --build-target datamgr_service`
- 运行聚焦测试：`./build.sh --product-name <product> --build-target datamgr_service_test`
- 格式化/lint：`git-clang-format` 或对照 `.clang-format` 验证
- API 兼容性检查：对照 CloudServiceStub Handler 编号表和参数签名，确认只增不减；检查错误码是否只追加不修改

### Task-specific checks

- IPC 接口变更 → 运行 API 兼容性检查，确认 Handler 编号和参数顺序不变
- 同步流程变更 → 构建 cloud 模块并运行 SyncManager 相关单元测试
- 策略变更 → 运行 NetworkSyncStrategy 单元测试
- 共享功能变更 → 运行 SharingCenter 相关测试
- 数据转换变更 → 运行 CloudDataTranslate/CloudValueUtil 单元测试，验证双向转换一致性
- 通知回调变更 → 构建 cloud 模块并检查 CloudNotifierProxy 回调调用方兼容性
- DFX/日志变更 → 运行相关故障/诊断测试
- 仅测试变更 → 运行变更的测试及至少一个相邻相关测试

### Done definition

任务完成仅在以下条件全部满足时：
- 请求的行为已实现。
- 相关构建/测试/lint/兼容性检查已执行，或已说明无法执行的原因。
- 最终回复包含：变更摘要、变更文件列表、验证结果、剩余风险。
- 不包含无关的格式化、重构或附带变更。
- 测试覆盖：修改和新代码有 UT 覆盖，新增外部接口有 FUZZ 测试覆盖。

### 测试约束

- 测试用例必须包含显式断言（EXPECT_XXX / ASSERT_XXX），禁止无断言测试。
- 辅助方法的断言必须在测试用例中，而非辅助方法内部。
- 禁止不可能失败的断言（如 `EXPECT_TRUE(true)`）。
- Mock 时使用统一 Mock 类，需要不同实现可继承扩展。
- 所有测试用例文档头必须包含 `@tc.author: agent`。
- UT 测试遵循 FIRST 原则，测试名称格式：被测方法_测试场景_预期结果。
