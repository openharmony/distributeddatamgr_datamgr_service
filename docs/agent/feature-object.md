# feature-object.md — Object 模块领域知识

本文件是 AI Agent 处理 Object 相关任务时的领域知识页。不要一开始就逐行精读全部内容；先看目录结构定位相关文件，按任务类型从知识路由表跳转到对应章节。

Agent 能自己探索出来的少写；Agent 猜不准、猜错代价高、团队必须统一执行的内容要写。

## 模块定位

Object 模块实现分布式对象数据同步的**服务端协调层**，在 OpenHarmony 源码树中的位置是：

```text
//foundation/distributeddatamgr/datamgr_service/services/distributeddataservice/service/object
```

核心职责：分布式对象存储管理、端端数据同步调度、Asset（资源）异步传输、数据变更通知分发、快照管理、DMS 事件处理。

最重要的架构边界：**服务端管理协调，通过 KvStoreNbDelegate 进行实际存储**。数据库句柄通过 `KvStoreDelegateManager` 获取（`ObjectStoreManager::GetKvStoreDelegateManager()`），使用后需及时释放，不得长期持有。

## 知识路由

按任务或问题类型决定下一步关注哪个文件或本文件哪个章节：

| 任务或问题 | 关注 |
| --- | --- |
| 同步行为变更 | `object_manager.cpp`（`SyncOnStore()`/`DoSync()`/`SyncCompleted()`）+ `SequenceSyncManager` |
| Asset 传输变更 | `object_asset_loader.cpp` + `asset_sync_manager.cpp` + 本文件Asset 传输 |
| 数据变更通知变更 | `object_data_listener.cpp` + `object_manager.cpp` `NotifyChange()` |
| 快照管理变更 | `object_snapshot.cpp` |
| DMS 事件处理变更 | `object_dms_handler.cpp` |
| 权限校验变更 | `object_service_impl.cpp` `IsBundleNameEqualTokenId()` |
| 错误码变更 | `object_common.h` `Status` 枚举 + 本文件错误码 |
| 元数据同步变更 | `object_manager.cpp` `IsNeedMetaSync()` |

在计划中声明：任务类别、已读文档、发现的约束、是否应使用特定 Skill/工作流。

## 硬约束

- 服务端管理协调，通过 KvStoreNbDelegate 进行实际存储；数据库句柄使用后需及时释放，不得长期持有。
- 所有端端同步必须经过 `SequenceSyncManager::AddNotifier()` 和 `ObjectStoreManager::DoSync()`，不得绕过。
- Asset 传输必须通过 `ObjectAssetLoader::Transfer()` 或 `TransferAssetsAsync()`，不得直接操作文件传输。
- 数据变更通知必须通过 `ObjectDataListener` 注册和分发，不得绕过。
- 不要长期持有 KvStoreNbDelegate 句柄作为成员变量。
- 不要变更错误码定义，除非任务明确要求。
- 不要变更公共 API 签名/错误码/权限行为，除非任务明确要求。
- 不要为通过测试删除日志/事件/错误码（含雷达上报）。
- 不要明文打印 udid/uuid/ip/mac/密钥/路径——用 `Anonymous()` 匿名化。
- 不要变更序列化格式或持久化数据结构，除非任务明确要求并提供迁移逻辑。
- 不要混用不同层的错误码（Status / DBStatus），必须经转换逻辑。

以下操作需先确认：

- 新增生产依赖 — 例如在 `service/object/BUILD.gn` 的 deps 中新增第三方库
- 变更公共 API 语义 — 例如修改 `object_service_stub.cpp` 中某个 IPC 接口的返回值含义或参数行为
- 变更权限模型或信任边界 — 例如修改 `object_service_impl.cpp` 中 `IsBundleNameEqualTokenId()` 的校验规则
- 变更协议兼容性或持久化数据格式 — 例如修改 SaveInfo 的序列化字段
- 删除兼容性适配或迁移逻辑 — 例如删除旧版本数据迁移代码
- 变更功能模块编译开关 — 例如修改编译配置

### Asset 传输

Asset 是分布式对象中的资源类型（如图片、文件等），需要在设备间异步传输：

- **Upload**：本设备将 Asset 推送到对端设备
- **Download**：本设备从对端设备拉取 Asset
- **TransferStatus**：Asset 传输状态枚举（STATUS_STABLE/STATUS_UPLOADING/STATUS_DOWNLOADING 等）

传输流程（对应 `object_manager.cpp` 的代码逻辑）：

1. Save 时发现数据包含 Asset → 进入 RestoreStatus 状态机
2. `PushAssets()` 发起 Upload，将 Asset 推送到目标设备
3. 目标设备 `ObjectAssetsRecvListener::OnStart()` 接收开始通知
4. 目标设备 `PullAssets()` 拉取 Asset 内容
5. 传输完成 `OnFinished()` → `NotifyAssetsReady()` 通知数据已就绪
6. 源设备 `WaitAssets()` 等待对端确认接收完成
7. 状态转为 ALL_READY → `NotifyDataChanged()` 通知应用数据已完整

不要随意变更 Asset 传输流程，配错会导致资源丢失或传输异常。

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
