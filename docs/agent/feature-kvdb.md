# feature-kvdb.md — KVDB 模块领域知识

Agent 能自己探索出来的少写；Agent 猜不准、猜错代价高、团队必须统一执行的内容要写。
通用约束见根 AGENTS.md §3。

## 1. Where to look

| 任务类型 | 先看哪里 |
|---|---|
| 同步行为 | `kvdb_service_impl.h/.cpp` + `kvstore_sync_manager.h/.cpp` |
| 冲突策略 | `kvdb_general_store.h/.cpp` |
| Store 创建 | `kvdb_service_impl.h/.cpp`（BeforeCreate/AfterCreate） |
| 权限/访问控制 | `kvdb_service_stub.h/.cpp` + `auth_delegate.h/.cpp` |
| Store 句柄 / AutoCache | `framework/include/store/auto_cache.h` + `framework/store/auto_cache.cpp`（KVDB Store 句柄获取、GC回收、观察者注册） |
| 数据迁移 | `upgrade.h/.cpp` |
| DFX 雷达 | `kv_radar_reporter.h` |
| 账号适配器 | `adapter/account/account_delegate_impl.h`（账号ID/用户查询/ foreground用户） |
| 设备适配器 | `adapter/communicator/device_manager_adapter.h`（设备UUID/网络ID转换/同账号判定） |
| 进程通信适配器 | `adapter/communicator/process_communicator_impl.h`（设备间数据通道/MTU大小） |
| 网络适配器 | `adapter/network/network_delegate_normal_impl.h`（网络可用性/网络类型） |

## 2. 知识路由

### 术语触发

当任务、issue、log、API、变更文件中出现以下术语时，先理解风险再规划：

| 术语 | 风险提示 | 动作 |
|---|---|---|
| seqId / 同步通知 | seqId == UINT64_MAX 是 DeviceMatrix 自动同步通知，非手动同步，DoComplete 跳过客户端回调 | 确认同步回调通知逻辑是否受影响 |
| CacheFlag / 句柄回收 | 控制句柄是否可被 AutoCache 回收，非数据缓存开关，命名易误导。长期持有句柄曾导致 crash | 读 `kvdb_service_impl.cpp` 中 SetCacheFlag/PublishCacheChange，确认修改的是回收行为而非缓存行为 |
| DBStatus / GeneralError / Status | 三层错误码体系不可混用，跨层必须经转换函数。ConvertDbStatus（HAP）和 ConvertDbStatusNative（非HAP）是两条独立路径，不可互相替代 | 对照 `general_error.h` 与 `kvdb_service_impl.cpp` 中转换函数，确认使用正确的转换路径 |
| Observer / 观察者指针 | Observer 历史上用裸指针存储导致 UAF，已改为智能指针。新增 Observer 时必须用 shared_ptr/weak_ptr，禁止裸指针 | 检查新增的 Observer 类是否使用智能指针管理生命周期 |
| DefaultImpl / stub | 条件编译关闭时适配器用 stub，行为完全不同（AccountDelegate 返回 "default"、NetworkDelegate IsNetworkAvailable 恒 false） | 修改适配器代码前检查 `datamgr_service.gni` 中开关状态；确认 mock 和真实适配器接口对齐 |

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