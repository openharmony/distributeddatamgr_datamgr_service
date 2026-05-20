# DFX 指南

## 适用范围

本文档描述可观测性、诊断、故障定位的规则和约定，是 DFX、日志、故障定位变更时的官方参考。

## DFX 三支柱架构

### Reporter 体系

```
Reporter (单例注册中心)
  ├── FaultReporter (故障上报)
  │     └── Report(ArkDataFaultMsg)
  ├── BehaviourReporter (行为上报)
  │     ├── Report(BehaviourMsg)
  │     └── UDMFReport(UdmfBehaviourMsg)
  └── StatisticReporter<T> (统计上报)
        └── Report(T stat)
```

实现层（Adapter）使用 `__attribute__((used))` 静态初始化注册：

```cpp
// adapter/dfx/src/reporter_impl.cpp
static bool g_isInit = ReporterImpl::Init();
// Init() 中注册具体 Reporter 实例到 Reporter 单例
```

### RadarReporter（云同步专用）

RAII 风格的雷达上报器，构造函数上报 `BEGIN`，析构函数上报 `END`：

- 事件名：`DISTRIBUTED_CLOUD_SYNC_BEHAVIOR`、`DISTRIBUTED_CLOUD_SHARE_BEHAVIOR`
- 域：`DISTRIBUTED_DATAMGR`
- 捕获字段：`ORG_PKG`、`FUNC`、`BIZ_SCENE`、`BIZ_STAGE`、`STAGE_RES`、`ERROR_CODE`、`HOST_PKG`、`LOCAL_UUID`、`CONCURRENT_ID`、`TRIGGER_MODE`、`WATER_VERSION`

## HiSysEvent 事件定义

事件定义在 `hisysevent.yaml` 中，域名为 `DISTDATAMGR`。

### 事件清单

| 事件名 | 类型 | 级别 | 参数 | DfxCode |
|--------|------|------|------|---------|
| `DATABASE_FAULT` | FAULT | CRITICAL | APP_ID, STORE_ID, MODULE_NAME, ERROR_TYPE | 950001102 |
| `DATABASE_SYNC_FAILED` | FAULT | CRITICAL | APP_ID, STORE_ID, MODULE_NAME, ERROR_TYPE | 950001112 |
| `DATABASE_CORRUPTED_FAILED` | FAULT | CRITICAL | APP_ID, STORE_ID, MODULE_NAME, ERROR_TYPE | 950001113 |
| `DATABASE_REKEY_FAILED` | FAULT | CRITICAL | APP_ID, STORE_ID, MODULE_NAME, ERROR_TYPE | 950001114 |
| `COMMUNICATION_FAULT` | FAULT | CRITICAL | ANONYMOUS_UID, APP_ID, STORE_ID, SYNC_ERROR_INFO | 950001103 |
| `VISIT_STATISTIC` | STATISTIC | MINOR | TAG, APP_ID, INTERFACE_NAME, TIMES | 950001105 |
| `TRAFFIC_STATISTIC` | STATISTIC | MINOR | TAG, APP_ID, ANONYMOUS_DID, SEND_SIZE, RECEIVED_SIZE | 950001106 |
| `API_PERFORMANCE_STATISTIC` | STATISTIC | MINOR | INTERFACES | 950001110 |
| `DATABASE_BEHAVIOUR` | BEHAVIOR | MINOR | ANONYMOUS_UID, APP_ID, STORE_ID, BEHAVIOUR_INFO | 950001115 |
| `UDMF_DATA_BEHAVIOR` | BEHAVIOR | MINOR | APP_ID, CHANNEL, DATA_SIZE, DATA_TYPE, OPERATION, RESULT | 950001116 |
| `OPEN_DATABASE_FAILED` | FAULT | CRITICAL | APP_ID, STORE_ID, ERROR_CODE | - |
| `ARKDATA_CLOUD_SYNC_FAULT` | FAULT | CRITICAL | FAULT_TIME, FAULT_TYPE, BUNDLE_NAME, MODULE_NAME, STORE_NAME, BUSINESS_TYPE, ERROR_CODE, APPENDIX | 950001117 |

### 映射链

```
DfxCodeConstant (数值码)
  → EVENT_COVERT_TABLE (hiview_adapter.cpp 中的映射表)
    → 事件字符串名 (hisysevent.yaml 中定义)
      → OH_HiSysEvent_Write() (C API 调用)
```

所有 HiSysEvent 上报都是**异步**的，使用 `ExecutorPool` 任务执行。

## HiLog 使用规范

### 日志宏

使用 `ZLOGD`、`ZLOGI`、`ZLOGW`、`ZLOGE` 宏，来自 `log_print.h`。

访问日志宏需要在 BUILD.gn 中添加：
```gn
external_deps = [ "kv_store:datamgr_common" ]
```

### 日志级别选择

| 级别 | 使用场景 |
|------|---------|
| `ZLOGD` | 调试信息，仅在开发阶段关注 |
| `ZLOGI` | 关键流程节点，如服务启动、Feature 注册、同步开始 |
| `ZLOGW` | 可恢复的异常，如重试、降级、兼容性处理 |
| `ZLOGE` | 错误，如数据库操作失败、同步失败、权限拒绝 |

### 敏感信息处理

- 设备 ID 使用 `Anonymous::GetAnonymous()` 脱敏
- 用户 ID 使用 `Anonymous` 工具处理
- 密钥和密码**绝不**出现在日志中

## 诊断工具

### hidumper 命令

```
hidumper -s 1301 -a "<选项>"
```

| 选项 | 短选项 | 功能 |
|------|--------|------|
| `--error-info` | `-e` | 显示最近错误信息（最多10条，含时间戳） |
| `--help-info` | `-h` | 显示帮助信息 |
| `--all-info` | `-a` | 转储所有诊断信息 |

### 错误追踪

`DumpHelper` 维护环形缓冲区（最多 10 条），使用 `AddErrorInfo(errorCode, errorInfo)` 记录错误，支持扩展注册新的 Dump Handler。

## DFX 变更规则

### 禁止事项

- 不要为通过测试而删除日志、事件、错误码或诊断信息
- 不要降低 FAULT 级别事件的级别
- 不要在日志中输出敏感信息（密钥、密码、未脱敏的设备 ID）
- 不要删除 `hisysevent.yaml` 中已有的事件定义
- 不要修改已有事件的参数类型和顺序

### 必须遵守

- 新增关键操作需添加对应的 DFX 上报
- 数据库操作失败必须上报 FAULT 事件
- 同步操作需记录开始和结束（含结果）
- 云同步操作使用 `RadarReporter` 进行完整的生命周期上报
- 新增 `hisysevent.yaml` 事件定义前必须先问人

### 新增 DFX 上报的流程

1. 如需新事件，在 `hisysevent.yaml` 中定义（需获批准）
2. 在 `adapter/dfx/src/dfx_code_constant.h` 中定义 DfxCode 常量
3. 在 `hiview_adapter.cpp` 的 `EVENT_COVERT_TABLE` 中添加映射
4. 在业务代码中调用 `Reporter` 对应接口
5. 确保上报是异步的（不阻塞业务线程）
6. 编写对应的测试验证上报行为
