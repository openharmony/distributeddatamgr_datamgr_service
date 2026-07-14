# feature-udmf.md - UDMF 领域 Agent 指南

本文档作为 ddms 代码中 UDMF/UTD 相关工作的模块级 AGENTS.md。本仓库`service/udmf/`和`service/utd/`主要为UDMF服务端代码，针对UDMF相关改动时优先阅读并遵守本文件。

## 1. 代码地图

UDMF 在 ddms 中属于 `services/distributeddataservice/service/` 下的服务层能力，整体分为以下几层：

| 层级 | 代码位置 | 职责 |
|---|---|---|
| Feature 注册层 | `udmf_service_impl.cpp`、`utd_service_impl.cpp` | 通过 `FeatureSystem::RegisterCreator` 注册 `udmf`/`utd` feature，随 `distributeddatasvc` 启动绑定 |
| IPC Stub 层 | `udmf_service_stub.*`、`utd_service_stub.*` | 校验 interface token、分发 IPC code、读写 `MessageParcel` |
| 业务编排层 | `udmf_service_impl.*` | 实现 UDMF服务端能力的主流程 |
| 存储适配层 | `udmf/store/` | 抽象 `Store`，用 `RuntimeStore` 封装 DistributedDB，管理 data/runtime/summary/local KV/sync/observer |
| 数据预处理层 | `udmf/preprocess/` | 填充 Runtime、解析和转换 URI、计算 Summary、处理 file/html record、执行序列化 |
| 权限策略层 | `udmf/permission/` | 校验 privilege、URI 授权、permission policy/mask/legacy policy 转换 |
| 生命周期层 | `udmf/lifecycle/` | 管理 drag 等 intention 的数据获取、过期、卸载清理、远端拉取 |
| 延迟数据层 | `udmf/delay_data/` | 管理 provider/acquire callback、阻塞等待、远端延迟数据同步和已同步设备缓存 |
| UTD 服务层 | `service/utd/` | 动态 UTD 类型注册/注销、notifier 分发、权限校验 |

构建开关在 `datamgr_service.gni`：`datamgr_service_udmf` 当前默认 `false`。开启后，`services/distributeddataservice/service/BUILD.gn` 会把 `service/udmf:udmf_server` 和 `service/utd:utd_server` 加入 `distributeddatasvc`。

## 2. 知识路由

### 2.1 先按任务类型读代码

| 遇到的问题/任务 | 先读 |
|---|---|
| UDMF Set/Get/Update/Delete 行为变化 | `udmf_service_impl.cpp`，再按调用进入 `preprocess/`、`store/`、`permission/` |
| IPC code、parcel 字段、接口新增 | `udmf_service_stub.*` 或 `utd_service_stub.*`，再读对应 fuzzer 和 stub 单测 |
| 数据落盘、summary/runtime entry、batch 查询 | `store/runtime_store.*`、`preprocess/data_handler.*`、`udmf_run_time_store_test.cpp` |
| DB error、DB corrupted、store 关闭/重开 | `RuntimeStore` 的 DB 状态处理、`UdmfServiceImpl::HandleDbError`、`CloseStoreWhenCorrupted`、`udmf_db_corruption_mock_test.cpp` |
| 权限、privilege、readAndKeep、tokenId/bundleName 校验 | `permission/`、`UdmfServiceImpl::Verify*Permission`、`udmf_checker_manager_test.cpp` |
| URI/file/html 授权或跨设备 URI 转换 | `preprocess/preprocess_utils.*`、`permission/permission_policy_utils.*`、`permission/uri_permission_manager.*` |
| drag 数据生命周期、超时清理、远端拉取 | `lifecycle/`、`store/drag_observer.*`、`udmf_observer_test.cpp` |
| delay data、provider callback、远端延迟数据 | `delay_data/`、`UdmfServiceImpl::SetDelayInfo`、`PushDelayData`、`GetDataIfAvailable`、`udmf_delay_data_container_test.cpp` |
| UTD 动态类型注册/注销 | `service/utd/`、`service/test/unittest/utd/`、`utdservice*_fuzzer` |
| 行为上报、雷达/DFX 日志 | `udmf_service_impl.cpp` 中 `Reporter`/radar 调用，必要时读 `adapter/dfx/` |
| 与路由头、远端同步能力相关 | `app/src/session_manager/route_head_handler_impl.cpp` 中 `UDMF_STORE_ID = "drag"` 相关逻辑 |

### 2.2 路径触发路由

- 改 `service/udmf/`：读本文档、`udmf_service_impl.h`、被改子目录对应头文件。
- 改 `service/utd/`：读本文档、`utd_service_impl.h`、`utd_service_stub.h`。
- 改 `service/test/unittest/udmf/`：读对应生产文件和 `BUILD.gn` 里的目标依赖。
- 改 `service/test/fuzztest/udmf*`：读对应 IPC/业务入口和同名非 concurrent fuzzer。
- 改 `datamgr_service.gni` 或 `service/BUILD.gn`：确认 `datamgr_service_udmf` 与 `udmf_server`/`utd_server` 是否同时受影响。

### 2.3 概念和黑话

| 术语 | 含义 |
|---|---|
| UDMF | Unified Data Management Framework，统一数据管理框架。这里实现服务端协调，不定义全部公共类型。 |
| UTD | Unified Type Descriptor，统一类型描述。ddms 中的 `service/utd/` 只负责动态类型注册和通知，不负责 UDMF 数据存储。 |
| data | 通常指`UnifiedData`。UDMF 传输的数据容器。比如拖拽场景下，data指本次拖拽的所有内容 |
| record | 通常指`UnifiedRecord`。UDMF 传输的具体数据记录。data与record的关系为1:N。比如拖拽10个文件，会生成1个data，data中含有10个record，分别对应具体的10个文件。 |
| entry | 通常指`UnifiedRecord`中的`entries`。UDMF 数据记录的具体类型，用于存储一个record中的不同数据类型。record与entry的关系为1:N。比如拖拽一个网页文本，生成的record中可能含有纯文本类型与HTML类型两个entry，数据接收方可以根据自身需求选择支持的数据类型来使用数据。 |
| UnifiedKey / udKey | UDMF 数据 key，常见形态 `udmf://<intention>/<bundle-or-app>/<id>`。不要手写拼接复杂规则，优先用 `UnifiedKey`。在数据写入UDMF数据库之后由服务端生成，不支持用户自定义。 |
| Intention | UDMF 场景路由键，例如 dataHub（即公共数据通路）、drag、system share、menu、picker。很多逻辑按 intention 生成不同分支，特别是在 drag 场景会包含较多定制逻辑。 |
| Runtime | 数据运行时元信息，包含 key、tokenId、createTime、dataStatus、permissionPolicyMode 等。 |
| Summary | 数据摘要，用于快速查询数据类型和大小等信息，和真实 data entry 分开存储。 |
| delay data | 拖拽场景下的延迟数据。先保存加载信息和 callback，真正数据稍后由 provider 推入。注意在同步接口中处理延迟数据时需要加入超时管控，按需增加重试机制。 |
| dataHub | 公共数据通路，也叫数据湖。该 intention 无任何写入、查询权限管控，但支持数据写入时设置 visibility可见范围。 |

## 3. 专家经验

### 3.1 架构边界

- UDMF 属于 service 层 feature，不要把 UDMF 业务实现下沉到 framework 层。
- `service/udmf` 与 `service/utd` 在 `datamgr_service_udmf` 下共同编译，但职责不能混：UDMF 管数据及通路，UTD 管类型描述。
- App 层不能直接依赖 UDMF 具体实现；通过 `FeatureSystem` 和服务绑定进入。
- 服务层 feature 之间不能形成直接业务依赖。需要跨模块交互时优先使用 framework 抽象、adapter、EventCenter 或已有服务接口。
- 不要绕过 `UdmfServiceStub`/`UtdServiceStub` 的 interface token、code 范围和 parcel 读写校验。
- 新增 IPC code 必须同步更新 interface code、`HANDLERS`、stub 单测、fuzzer，并确认客户端接口兼容。
- 新增异步或周期任务优先复用 `ExecutorPool`，不要在 UDMF 内新建独立线程。
- 日志使用 `ZLOG*`；key、bundle、URI、deviceId、路径等敏感或半敏感信息按现有匿名化/公开标记规则处理。
- 不要绕过 `RuntimeStore` 直接操作 DistributedDB delegate；DB status、summary/runtime entries、observer、corrupted 状态都集中在 `RuntimeStore`。
- 不要把 `RuntimeStore` 或 `StoreCache` 取得的 store 长期保存成业务对象成员。需要关闭或移除 store 时走 `StoreCache::CloseStores`、`RemoveStore` 或 DB 损坏处理路径。

### 3.2 业务边界
- 不要直接手写复杂 udKey 规则；优先使用 `UnifiedKey`、`Runtime`、`UD_INTENTION_MAP` 和 UDMF core helper。
- drag 是特殊 intention，仅 drag 支持URI的代理授权。代理授权为系统中的高风险操作，修改时要注意防止引入安全漏洞，如越权、路径穿越等。
- drag 在业务场景中性能敏感，修改时不能采用任何性能大幅度损耗的方案。
- UDMF 中仅 drag 支持跨设备能力，其余功能、intention 均不应该增加跨设备相关逻辑。
- 新增的 intention 需要用户显示指定其所需要的接口范围以及数据生命周期管理策略，不能随意follow现有代码能力。
- 所有安全性校验必须在服务端进程中处理，不相信任何通过IPC接口和数据库中写入的tokenId/bundleName/appId/appIdentify等，按照默认所有请求都不可信的严格方式进行校验。

## 4. 编译和测试方法

### 4.1 最小验证方法

文档或注释改动：

```bash
git diff -- docs/agent/feature-udmf.md
```

UDMF/UTD C++ 代码改动的最低验证：

```bash
./build.sh --product-name <product> --build-target datamgr_service --fast-rebuild
```

如果改了 GN、feature 注册、公共依赖或跨目录接口，不要只跑单测，至少构建完整服务目标：

```bash
./build.sh --product-name <product> --build-target datamgr_service
```

复杂验证可以配合 skill：

- 需要生成或扩展 OpenHarmony C++ UT：使用 `oh-ut-generator`。
- 需要编译、部署、运行 UT/FuzzTest：使用 `openharmony-ut`。
- 需要完整 XTS 编译运行：使用 `oh-xts-build-run`。
- 遇到非预期失败：先用 `superpowers:systematic-debugging` 做根因定位，再修改。
