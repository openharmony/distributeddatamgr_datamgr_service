# API 兼容性

## 适用范围

本文档描述公共 API 和 SDK 行为的兼容性规则，是 API 变更时的官方参考。

## 公共 API 层次

本服务对外暴露的 API 分为以下层次：

| 层次 | 位置 | 使用者 | 兼容性要求 |
|------|------|--------|-----------|
| NAPI/JS API | 应用开发接口 | 第三方应用 | 最高，不可破坏性变更 |
| IPC 接口 | `*service_stub.h` | 系统内其他进程 | 高，版本兼容性协议 |
| Inner API (Framework 导出) | `framework/include/` | 系统内同进程模块 | 中，需要评估影响 |
| 内部接口 | 各模块内部实现 | 本模块内部 | 低，但需保持功能一致 |

## IPC 接口结构

每个 Feature 的 IPC 接口遵循以下模式：

```
Feature Service Interface (IDL 定义)
  │
  ├── FeatureStub (服务端 Stub)
  │     └── OnRemoteRequest(code, data, reply, option)
  │           ├── code = 命令码，不可随意修改
  │           ├── data = 入参序列化
  │           └── reply = 出参序列化
  │
  └── FeatureProxy (客户端 Proxy)
        └── SendRequest(code, data, reply, option)
```

命令码（`code`）是 IPC 兼容性的关键。修改命令码、参数顺序、序列化格式都会破坏二进制兼容性。

## 流控规格

### KvStore 接口流控

| 接口类型 | 1秒限制 | 1分钟限制 |
|---------|--------|----------|
| KvStore 操作 | 1000 次 | 10000 次 |
| KvManager 操作 | 50 次 | 500 次 |

流控实现在 `framework/include/flow_control_manager/flow_control_manager.h`。

### RDB 同步流控

| 限制类型 | 上限 |
|---------|------|
| 单应用并发同步 | 5 次 (`SYNC_APP_LIMIT_TIMES`) |
| 全局并发同步 | 20 次 (`SYNC_GLOBAL_LIMIT_TIMES`) |

流控实现在 `service/rdb/rdb_flow_control_manager.h`。

## KV 数据模型规格

### 数据库类型

| 类型 | Key 上限 | Value 上限 | 说明 |
|------|---------|-----------|------|
| 设备协同数据库 | 896 Byte | 4 MB | Key 拼接 DeviceID，按设备隔离 |
| 单版本数据库 | 1 KB | 4 MB | 每 Key 只保留一个条目 |

### 通用限制

- 每个程序最多同时打开 **16 个 DB**
- 仅支持 KV 数据模型，不支持外键、触发器
- 不支持自定义冲突解决策略（基于时间戳，取较大值）
- 仅支持最终一致性，不支持强一致性

## 兼容性变更规则

### 不可破坏性变更（除非任务明确要求）

- IPC 命令码（`code`）的数值
- IPC 参数序列化顺序和格式
- 公共 API 签名（函数名、参数、返回值）
- 错误码的定义和含义
- 权限名称和行为
- 生命周期语义（如 `OnBind`/`OnAppExit` 的调用时序和条件）
- 持久化数据格式（`StoreMetaData` 的 JSON 序列化格式）
- 元数据版本号（当前 `CURRENT_VERSION=5`，`StoreMetaData` 版本 `0x03000006`）

### 需要评估影响的变更

- Feature 名称字符串（如 `"kv_store"`、`"rdb"`）
- `StoreMetaData` 新增字段（需要设置默认值和版本兼容）
- `hisysevent.yaml` 中的事件定义（参数变更影响诊断工具链）
- 配置文件 `conf/config.json` 的结构变更
- Feature 开关默认值变更

### 可以安全变更

- 内部实现逻辑（不影响外部行为）
- 日志级别和内容（不删除诊断关键日志）
- 测试代码
- 内部类和方法的重构

## 版本兼容处理模式

### 元数据版本升级

新增元数据字段时的标准流程：

1. 新增字段设置合理默认值
2. 递增 `VersionMetaData::CURRENT_VERSION`
3. 在 `Unmarshal()` 中处理旧版本缺失字段的默认值
4. 确保新代码能正确处理旧版本数据

### IPC 版本兼容性协议

设备间同步时使用 `CapMetaData` 进行能力协商：

- `CapMetaData.version` 标识设备支持的能力版本（当前版本 3）
- 新能力需使用版本号判断是否启用
- 低版本设备不应收到其无法处理的数据

### DeviceMatrix 版本兼容

`DeviceMatrix` 使用版本号 `CURRENT_VERSION=3` 处理跨设备兼容：

- `ConvertDynamic()` / `ConvertStatics()` 处理版本差异
- 广播数据中携带版本信息
- 新增掩码位不影响旧版本解析

## 变更检查清单

进行 API 变更时，逐项确认：

- [ ] 是否修改了 IPC 命令码？
- [ ] 是否修改了参数序列化格式？
- [ ] 是否修改了公共头文件中的接口定义？
- [ ] 是否修改了错误码定义？
- [ ] 是否修改了权限名称？
- [ ] 是否修改了持久化数据格式？
- [ ] 是否修改了 `hisysevent.yaml`？
- [ ] 是否修改了 Feature 名称字符串？
- [ ] 是否新增了 `StoreMetaData` 字段？
- [ ] 是否修改了流控阈值？
- [ ] 元数据版本号是否需要递增？
- [ ] 低版本设备能否正确处理新数据？
