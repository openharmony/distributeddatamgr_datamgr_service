# feature-kvdb.md — KVDB 模块领域知识

Agent 能自己探索出来的少写；Agent 猜不准、猜错代价高、团队必须统一执行的内容要写。
通用约束见根 AGENTS.md §3。

## 1. Where to look

| 任务类型 | 先看哪里 |
|---|---|
| 同步行为 | `kvdb_service_impl.h/.cpp` + `kvstore_sync_manager.h/.cpp` |
| 冲突策略 | `kvdb_general_store.h/.cpp` |
| Store 创建 | `kvdb_service_impl.h/.cpp`（BeforeCreate/AfterCreate） |
| 权限/访问控制 / IPC 接口码 | `kvdb_service_stub.h/.cpp` + `auth_delegate.h/.cpp` |
| Store 句柄 / AutoCache | `framework/include/store/auto_cache.h` + `framework/store/auto_cache.cpp`（KVDB Store 句柄获取、GC回收、观察者注册） |
| 数据迁移 | `upgrade.h/.cpp` |
| DFX 雷达 | `kv_radar_reporter.h` |
| 账号适配器 | `adapter/account/account_delegate_impl.h`（账号ID/用户查询/ foreground用户） |
| 设备适配器 | `adapter/communicator/device_manager_adapter.h`（设备UUID/网络ID转换/同账号判定） |
| 进程通信适配器 | `adapter/communicator/process_communicator_impl.h`（设备间数据通道/MTU大小） |
| 网络适配器 | `adapter/network/network_delegate_normal_impl.h`（网络可用性/网络类型） |

## 2. 知识路由

### 术语触发

当任务、issue、log、API、变更文件中出现以下术语时，先理解风险再规划。

| 术语 | 风险 | 检查 |
|---|---|---|
| DEVICE_COLLABORATION / SINGLE_VERSION | 冲突策略选择；SINGLE_VERSION 用 LAST_WIN 静默丢数据 | push 拦截器改了必须同步更新 receive |
| DeviceMatrix | 位掩码变更追踪与广播协调；元数据未同步时数据同步静默跳过 | 检查 IsNeedMetaSync |
| BeforeCreate / AfterCreate | Store 创建必经两阶段，中间插入加密密码；缺前者丢兼容检查，缺后者留不一致状态 | 两个阶段都不可跳过 |
| StoreMetaData / StoreMetaDataLocal | 分布式 vs 本地，混淆会同步私有数据 | 删除时必须清理两种+密钥+策略 |
| SyncMode | 两个枚举；NEARBY_END(=3) 分隔端端和端云 | 查 ConvertGeneralSyncMode |
| CleanMode | 数据清除类别；CLOUD_INFO 对非 public 只删标记不删数据 | 对齐 isPublic |
| AuthDelegate | 设备间数据访问授权；CheckAccess 第二 bool 错误致数据前缀错乱 | 检查第二 bool 语义 |

## 3. 硬约束

### 禁止事项

- NEVER 混用三层错误码（DBStatus/GeneralError/Status），必须经转换函数。
- NEVER 为通过测试删除 RADAR_REPORT 上报点。
- NEVER 变更冲突策略默认值，除非任务明确要求（配错会静默丢数据）。
- NEVER 修改 DEVICE_COLLABORATION key 的 push 拦截器而不同步更新 receive 拦截器。
- NEVER 删除密码 buffer 清零逻辑（`password.assign(password.size(), 0)`）。

### 必须先问人

- 变更公共 API 签名、错误码或权限行为。
- 变更冲突策略默认值。
- 变更 AUTH_GROUP_TYPE 授权组类型值或权限模型。
- 变更 StoreMetaData 持久化格式。

## 4. 验证

> 通用构建/lint/Done 定义见根 AGENTS.md §4。

隐私日志检查：

```powershell
rg -n "ZLOG[IWE].*%{public}.*(udid|uuid|ip|mac|path|password|pwd)" services/distributeddataservice/service/kvdb services/distributeddataservice/adapter
```
稳定性排查 / 日志打印排查 / 安全编码自检见：

.gitee/PULL_REQUEST_TEMPLATE.zh-CN.md`。