# feature-data-share.md — DataShare 模块领域知识

本文件是 AI Agent 处理 DataShare 相关任务时的领域知识页。不要一开始就逐行精读全部内容；先看目录结构定位相关文件，按任务类型从知识路由表跳转到对应章节。

## 模块定位

DataShare 模块实现跨应用数据共享的**服务端协调层**，在 OpenHarmony 源码树中的位置是：

```text
//foundation/distributeddatamgr/datamgr_service/services/distributeddataservice/service/data_share
```

核心职责：跨应用数据共享管理、静默访问（SilentAccess）、数据订阅分发、权限校验、Proxy 数据管理、Extension 连接管理、数据发布与获取。

最重要的架构边界：**服务端管理协调，通过 DBDelegate/KvDBDelegate 进行实际存储**。数据库句柄通过 `DBDelegate::Create()` 获取，使用后需及时释放，不得长期持有。

## 知识路由

按任务或问题类型决定下一步关注哪个文件或本文件哪个章节：

| 任务或问题 | 关注 |
| --- | --- |
| 权限校验变更 | `strategies/general/permission_strategy.cpp` + `data_share_service_impl.cpp` `VerifyPermission()` |
| 静默访问开关变更 | `data_share_silent_config.cpp` + `data_share_service_impl.cpp` `EnableSilentProxy()` |
| RDB 订阅变更 | `subscriber_managers/rdb_subscriber_manager.cpp` + `strategies/subscribe_strategy.cpp` |
| 发布数据变更 | `strategies/publish_strategy.cpp` + `data/published_data.cpp` |
| 获取数据变更 | `strategies/get_data_strategy.cpp` + `data/published_data.cpp` |
| Template 管理变更 | `strategies/template_strategy.cpp` + `data/template_data.cpp` |
| Proxy 数据变更 | `common/proxy_data_manager.cpp` + `subscriber_managers/proxy_data_subscriber_manager.cpp` |
| Extension 连接变更 | `common/extension_ability_manager.cpp` + `common/extension_connect_adaptor.cpp` |
| 配置加载变更 | `data_provider_config.cpp` + `strategies/general/load_config_*.cpp` |
| 跨权限策略变更 | `strategies/general/cross_permission_strategy.cpp` |
| DFX/故障上报变更 | `dfx/hiview_adapter.cpp` + `dfx/hiview_fault_adapter.cpp` |

在计划中声明：任务类别、已读文档、发现的约束、是否应使用特定 Skill/工作流。

## 硬约束

- 服务端管理协调，通过 DBDelegate/KvDBDelegate 进行实际存储；数据库句柄使用后需及时释放，不得长期持有。
- 所有权限校验必须通过 `PermissionStrategy` 或 `VerifyPermission()`，不得绕过。
- 静默访问开关状态通过 `DataShareSilentConfig` 管理，不得绕过配置直接访问。
- 数据订阅必须通过 `RdbSubscriberManager`/`PublishedDataSubscriberManager`/`ProxyDataSubscriberManager`，不得绕过。
- Extension 连接必须通过 `ExtensionAbilityManager`，不得直接操作 AMS。
- 不要长期持有 DBDelegate 句柄作为成员变量。
- 不要变更错误码定义，除非任务明确要求。
- 不要变更公共 API 签名/错误码/权限行为，除非任务明确要求。
- 不要为通过测试删除日志/事件/错误码（含雷达上报）。
- 不要明文打印 udid/uuid/ip/mac/密钥/路径——用匿名化处理。
- 不要变更序列化格式或持久化数据结构，除非任务明确要求并提供迁移逻辑。

以下操作需先确认：

- 新增生产依赖 — 例如在 `service/data_share/BUILD.gn` 的 deps 中新增第三方库
- 变更公共 API 语义 — 例如修改 `data_share_service_stub.cpp` 中某个 IPC 接口的返回值含义或参数行为
- 变更权限模型或信任边界 — 例如修改 `permission_strategy.cpp` 中的校验规则
- 变更协议兼容性或持久化数据格式 — 例如修改 PublishedDataNode 的序列化字段
- 删除兼容性适配或迁移逻辑 — 例如删除旧版本数据迁移代码
- 变更功能模块编译开关 — 例如修改编译配置

### 静默访问（SilentAccess）

静默访问是跨应用数据共享的核心机制，允许访问方在不启动提供方进程的情况下访问数据：

- **EnableSilentProxy**：启用静默访问开关
- **GetSilentProxyStatus**：获取静默访问状态
- **DataShareSilentConfig**：静默访问配置管理类

访问流程（对应 `data_share_service_impl.cpp` 的代码逻辑）：

1. 调用方通过 URI 访问数据
2. `DataProviderConfig::GetProviderInfo()` 解析提供方信息
3. `PermissionStrategy` 校验访问权限
4. 若启用静默访问 → 通过 `DBDelegate` 直接访问数据
5. 若未启用 → 通过 `ExtensionAbilityManager` 连接提供方 Extension
6. 返回查询结果

不要随意变更静默访问流程，配错会导致权限泄露或访问失败。

### 数据订阅管理

DataShare 支持三种订阅类型：

#### RDB 订阅

- 通过 `RdbSubscriberManager` 管理
- 支持 Template 模板订阅
- 订阅流程：Add → Enable/Disable → Delete
- 数据变更时通过 `Emit()` 通知观察者

#### Published 数据订阅

- 通过 `PublishedDataSubscriberManager` 管理
- 基于键值对的数据发布/订阅
- 发布方调用 `Publish()`，订阅方通过 `SubscribePublishedData()` 订阅

#### Proxy 数据订阅

- 通过 `ProxyDataSubscriberManager` 管理
- 支持跨应用数据代理
- 提供 `PublishProxyData()`、`GetProxyData()`、`DeleteProxyData()` 等操作
- 支持多值追加模式（MultiValues）

不要绕过订阅管理器直接操作订阅数据。

### 策略模式

DataShare 使用策略模式处理各类操作，位于 `strategies/` 目录：

| 策略类 | 文件 | 职责 |
| --- | --- | --- |
| SubscribeStrategy | subscribe_strategy.cpp | RDB 数据订阅 |
| UnsubscribeStrategy | unsubscribe_strategy.cpp | 取消订阅 |
| PublishStrategy | publish_strategy.cpp | 发布数据 |
| GetDataStrategy | get_data_strategy.cpp | 获取发布的数据 |
| TemplateStrategy | template_strategy.cpp | 模板管理 |
| RdbNotifyStrategy | rdb_notify_strategy.cpp | RDB 数据变更通知 |
| PermissionStrategy | strategies/general/permission_strategy.cpp | 权限校验 |
| CrossPermissionStrategy | strategies/general/cross_permission_strategy.cpp | 跨权限校验 |

所有策略通过 `SeqStrategy` 组合执行，按责任链顺序调用。

不要绕过策略直接执行核心逻辑。

### Extension 连接管理

当静默访问未启用时，需要连接提供方的 DataShare Extension：

- `ExtensionAbilityManager`：管理 Extension 连接生命周期
- `ExtensionConnectAdaptor`：连接适配器
- 支持延迟断开（`DelayDisconnect()`）避免频繁连接/断开

连接流程：

1. `ConnectExtension()` 发起连接请求
2. 等待 Extension 启动并返回回调
3. 执行数据操作
4. 延迟断开连接（默认 5 秒，最大 30 秒）

不要直接调用 AMS 接口，必须通过 `ExtensionAbilityManager`。

### Proxy 数据

Proxy 数据是 DataShare 的跨应用数据代理机制：

- **PublishProxyData**：发布代理数据
- **GetProxyData**：获取代理数据
- **DeleteProxyData**：删除代理数据
- **SubscribeProxyData**：订阅代理数据变更
- **PutValue/RemoveValue/GetValues**：多值模式操作

存储通过 `KvDBDelegate` 实现，数据结构为 `ProxyDataNode`。

不要绕过 `ProxyDataManager` 直接操作代理数据。

### 验证

声明完成前，在源码树根目录检查和构建：

```powershell
# 构建全量服务
./build.sh --product-name <product> --build-target datamgr_service
# 运行测试
./build.sh --product-name <product> --build-target datamgr_service_test
```
环境变量：`<product>` — 传给 `./build.sh --product-name` 的产品占位符（示例 `rk3568`）。

### Done 定义

任务完成仅在以下条件全部满足时：

- 请求的行为已实现。
- 相关构建/测试/安全自检/兼容性检查已执行，或已说明无法执行的原因。
- 最终回复包含：变更摘要、变更文件列表、验证结果、剩余风险。
- 不包含无关的格式化、重构或附带变更。

完成任务时，最终回复应包含：变更摘要、变更文件列表、验证命令与结果、兼容性/权限/DFX 影响（如相关）、剩余风险或后续事项。