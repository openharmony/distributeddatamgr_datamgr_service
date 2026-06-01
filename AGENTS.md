# AGENTS.md

## 1. 代码地图

本 AGENTS.md 适用于仓库根目录。子目录可能包含更具体的规则文件。

本仓库实现 OpenHarmony **分布式数据管理服务**（distributeddatamgr_datamgr_service），核心职责是元数据管理、数据同步协调、静默访问、权限管理和备份恢复。最重要的架构边界是**客户端存储、服务端管理**的分离：客户端负责实际数据存储，本服务负责管理协调。

**三层架构**：
- **应用层** `services/distributeddataservice/app/`：服务启动入口、FeatureSystem 分发、事件分发
- **服务层** `services/distributeddataservice/service/` + `adapter/`：业务逻辑实现、同步协调、权限管理
- **框架层** `services/distributeddataservice/framework/`：抽象接口定义、依赖注入容器、通用能力管理

关键路径：

| 路径 | 职责 | 变更风险 |
|---|---|---|
| `services/distributeddataservice/app/` | 服务入口、Feature 分发 | 架构边界，影响全局启动 |
| `services/distributeddataservice/service/rdb/` | RDB 功能模块实现 | 高频修改，端端/端云同步核心 |
| `services/distributeddataservice/service/kvdb/` | KVDB 功能模块实现 | 高频修改，键值同步核心 |
| `services/distributeddataservice/service/cloud/` | 云端功能模块实现 | 端云同步、云数据管理 |
| `services/distributeddataservice/service/data_share/` | 数据共享功能模块 | 静默访问、跨应用共享 |
| `services/distributeddataservice/service/object/` | 分布式对象功能模块 | 对象生命周期管理 |
| `services/distributeddataservice/adapter/` | 系统服务适配器 | 依赖注入实现层 |
| `services/distributeddataservice/framework/` | 框架层接口与容器 | 接口定义、全局约束 |
| `rust/ylong_cloud_extension/` | Rust 云端扩展 | 云端协议实现 |
| `datamgr_service.gni` | 功能模块编译开关 | 编译配置入口 |

Where to look：

- 功能模块行为变更 → `services/distributeddataservice/service/<domain>/` + `docs/agent/domain-<domain>.md`
- 端云功能模块行为变更 → `docs/agent/cloud/AGENTS.md` + `docs/agent/cloud/domain-cloud.md`
- 适配器或依赖注入变更 → `services/distributeddataservice/adapter/` + `docs/agent/architecture-map.md`
- 框架层接口变更 → `services/distributeddataservice/framework/` + `docs/agent/architecture-map.md`
- 测试变更 → 先检查同目录下已有测试的模式和覆盖范围
- 编译或 Feature 开关 → `datamgr_service.gni`

## 2. 知识路由

以下文档不是可选背景阅读。当任务命中对应类别时，必须在规划前阅读匹配文档。

### Task-based routing

- 架构或模块边界变更 → 阅读 `docs/agent/architecture-map.md`
- DFX、日志、故障归因变更 → 阅读 `docs/agent/dfx-guidelines.md`
- 同步行为变更 → 阅读 `docs/agent/domain-sync.md`
- 云端行为变更 → 阅读 `docs/agent/cloud/domain-cloud.md`
- 数据共享或静默访问变更 → 阅读 `docs/agent/domain-data-share.md`
- UDMF 行为变更 → 阅读 `docs/agent/domain-udmf.md`

### Path-based routing

- `services/distributeddataservice/service/rdb/` → `docs/agent/domain-sync.md`
- `services/distributeddataservice/service/kvdb/` → `docs/agent/domain-sync.md`
- `services/distributeddataservice/service/cloud/` → `docs/agent/cloud/domain-cloud.md`
- `services/distributeddataservice/service/data_share/` → `docs/agent/domain-data-share.md`
- `services/distributeddataservice/service/object/` → `docs/agent/domain-sync.md`
- `services/distributeddataservice/service/udmf/` → `docs/agent/domain-udmf.md`
- `services/distributeddataservice/adapter/` → `docs/agent/architecture-map.md`
- `services/distributeddataservice/framework/` → `docs/agent/architecture-map.md`
- `interfaces/` → `docs/agent/api-compatibility.md`

### Vocabulary-based routing

当任务、issue、日志、API 名称或变更文件中出现以下术语时，在规划前阅读链接文档：

| 术语 | 风险提示 | 阅读 |
|---|---|---|
| FeatureSystem / Feature | 功能模块注册和生命周期，不得直接跨模块依赖 | `docs/agent/architecture-map.md` |
| 静默访问 / SilentAccess | 跨应用数据共享，不启动提供方进程 | `docs/agent/domain-data-share.md` |
| 端云同步 / CloudSync | 设备-云端数据同步协议 | `docs/agent/cloud/domain-cloud.md` |
| 版本兼容性协议 / API 兼容性 | 公共行为和兼容性边界 | `docs/agent/api-compatibility.md` |
| DFX / HiLog / ZLOG | 可观测性、诊断、故障归因 | `docs/agent/dfx-guidelines.md` |
| 依赖注入 / Delegate | 框架层接口，不得直接依赖适配器实现 | `docs/agent/architecture-map.md` |
| ExecutorPool | 异步操作执行器，不得启动独立线程 | `docs/agent/architecture-map.md` |
| Serializable | 统一序列化机制，不得使用外部依赖 | `docs/agent/architecture-map.md` |
| UDMF | 统一数据管理框架，系统级数据标准 | `docs/agent/domain-udmf.md` |

在计划中声明：
- 任务类别
- 已读文档
- 发现的约束
- 是否应使用特定 Skill/工作流

## 3. 约束边界

### 架构不变量

- 公共 API 表达稳定的能力意图，不是内部实现细节。
- 框架层仅定义接口，系统服务调用由适配器完成。
- 应用层通过 FeatureSystem 分发，不感知功能模块实现。
- 功能模块之间不得相互依赖，每个模块支持配置和排除编译。
- 服务端通过 AutoCache 统一获取数据库句柄，不得使用客户端打开数据库。
- 服务端不得长期持有 AutoCache 获取的句柄，使用后尽快释放。
- 序列化和反序列化统一使用 Serializable，不得引入外部依赖。
- 异步操作使用 ExecutorPool，不得启动独立线程。

### Do not

- 不要绕过权限校验或混淆认证与信任状态。
- 不要在框架层直接调用系统服务。
- 不要在应用层直接依赖功能模块实现类。
- 不要让功能模块之间产生直接依赖。
- 不要长期持有 AutoCache 获取的数据库句柄作为成员变量。
- 不要为通过测试删除日志、事件、错误码或诊断信息。
- 不要直接修改 generated files，修改 source of truth 后重新生成。
- 不要在没有显式批准的情况下引入新的生产依赖。
- 不要变更公共 API 签名、错误码、权限行为或生命周期语义（除非任务明确要求）。
- 不要在异步操作中捕获 this（避免 UAF）。
- 不要直接测试私有方法，应通过公共接口覆盖。

### Ask before

- 添加新的第三方依赖。
- 变更公共 API 语义。
- 变更权限模型或信任边界。
- 变更协议兼容性或持久化数据格式。
- 删除兼容性适配或迁移逻辑。
- 变更功能模块编译开关配置。
- 执行可能影响连接设备的操作。

## 4. 验证

### 最小验证

- 格式化/lint：检查 .clang-format 和 rustfmt.toml 配置合规
- 构建当前模块：`./build.sh --product-name <product> --build-target datamgr_service`
- 运行聚焦测试：`./build.sh --product-name <product> --build-target datamgr_service_test`
- API 兼容性检查：检查公共接口签名和错误码是否保持兼容

### 任务级验证

- 公共 API 变更 → 运行 API 兼容性检查并更新 API 文档
- C++ 功能模块变更 → 构建受影响模块并运行相关单元测试
- 适配器/依赖注入变更 → 构建完整模块并运行集成测试
- DFX/日志变更 → 运行相关故障/诊断测试
- 同步行为变更 → 运行聚焦集成或冒烟测试
- Rust 扩展变更 → 构建并运行 ylong_cloud_extension 测试
- 仅测试变更 → 运行变更的测试及至少一个相邻相关测试

### Done 定义

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