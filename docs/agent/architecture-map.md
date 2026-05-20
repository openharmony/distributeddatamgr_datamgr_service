# 架构图

## 适用范围

本文档描述 `services/distributeddataservice/` 子系统的架构设计，是架构分层、模块边界变更时的官方参考。

## 核心架构原则

本服务采用**客户端存储、服务端管理**的分离架构：

- **客户端（应用进程）**：负责实际数据存储和访问，数据存储在应用本地数据库
- **服务端（本服务）**：负责元数据管理、数据同步协调、静默访问、权限管理、备份恢复等管理职能

数据隔离基于 **(用户, 应用, 数据库)** 三元组。

## 三层架构

```
┌─────────────────────────────────────────────────────────┐
│  App 层 (app/)                                           │
│  服务启动入口，Feature 分发，系统事件分发                      │
│  产物: libdistributeddataservice.z.so                     │
│  不可直接依赖 Feature 实现，使用 FeatureSystem 统一管理       │
├─────────────────────────────────────────────────────────┤
│  Service 层 (service/) + Adapter 层 (adapter/)           │
│  业务逻辑实现 + 系统服务适配                                   │
│  产物: libdistributeddatasvc.z.so (胶水层聚合)             │
│  各 Feature 之间不得相互依赖                                 │
├─────────────────────────────────────────────────────────┤
│  Framework 层 (framework/)                                │
│  抽象接口定义，依赖注入容器，通用能力                            │
│  产物: libdistributeddatasvcfwk.z.so                      │
│  独立共享库，零适配器依赖，仅定义接口                            │
└─────────────────────────────────────────────────────────┘
```

### 构建依赖关系

```
app/BUILD.gn (最终产物)
  ├── framework:distributeddatasvcfwk
  └── service:distributeddatasvc (胶水层)
        ├── adapter/account
        ├── adapter/communicator
        ├── adapter/dfx
        ├── adapter/network
        ├── adapter/power_manager
        ├── adapter/qos
        ├── adapter/utils
        ├── framework:distributeddatasvcfwk
        ├── service/bootstrap
        ├── service/kvdb       (条件编译)
        ├── service/rdb        (条件编译)
        ├── service/cloud      (条件编译)
        ├── service/object     (条件编译)
        ├── service/data_share  (条件编译)
        └── service/udmf       (条件编译)
```

## 依赖注入机制

三段式模式：**接口在 Framework 层定义 → Adapter 实现 → 注册到 Framework → Service 使用**

```
Framework 层                    Adapter 层                     使用方
┌──────────────┐               ┌───────────────────┐          ┌─────────────────┐
│ AccountDelegate│              │ NormalAccountDelegate│         │ AccountDelegate::│
│ {              │              │  : public AccountDelegate│     │  GetInstance()  │
│   static *inst │◄─ Register ──│  { /* 实际实现 */ }   │         │    ->DoSomething()│
│   Register()   │              │                     │          └─────────────────┘
│   GetInstance()│              │ 编译单元静态初始化:      │               ▲
│ }              │              │ static Normal impl;  │               │
└──────────────┘               │ AccountDelegate::    │         不关心具体实现
                               │   Register(&impl)    │
                               └───────────────────┘
```

关键 Delegate 接口：

| 接口 | 职责 | 注册方式 |
|------|------|---------|
| `AccountDelegate` | 账户事件、用户切换 | `RegisterAccountInstance()` |
| `NetworkDelegate` | 网络状态变更 | `RegisterNetworkInstance()` |
| `DeviceManagerDelegate` | 设备上下线、设备信息 | `RegisterInstance()` |
| `Reporter` | DFX 上报（Fault/Behaviour/Statistic） | `RegisterReporterInstance()` |
| `CloudServer` | 云驱动注册入口 | `RegisterCloudInstance()` |

适配器使用 `__attribute__((used))` 静态初始化在 `main()` 前自动注册。Adapter 的 BUILD.gn 使用 GN 条件判断决定链接真实实现还是空实现（降级模式）。

## FeatureSystem（特性管理）

### 注册机制

每个 Feature 使用静态工厂模式自注册：

```cpp
// 编译单元静态初始化时自动注册
__attribute__((used)) KVDBServiceImpl::Factory KVDBServiceImpl::factory_;
// Factory 构造函数中:
FeatureSystem::GetInstance().RegisterCreator("kv_store", creator);
AutoCache::GetInstance().RegCreator(KvStoreType::SINGLE_VERSION, storeCreator);
```

已注册的 Feature 名称：

| Feature 名称 | 实现位置 | 绑定模式 |
|-------------|---------|---------|
| `kv_store` | service/kvdb/ | BIND_LAZY |
| `rdb` | service/rdb/ | BIND_LAZY |
| `cloud_db` | service/cloud/ | BIND_NOW |
| `data_object` | service/object/ | BIND_LAZY |
| `data_share` | service/data_share/ | BIND_LAZY |
| `udmf` | service/udmf/ | BIND_LAZY |
| `utd` | service/utd/ | BIND_LAZY |

### 绑定模式

- **BIND_NOW**：服务启动时（`OnStart()`）立即创建并初始化
- **BIND_LAZY**：首次调用时按需创建

### 生命周期回调

FeatureSystem 向各 Feature 分发的系统事件：

| 回调 | 触发时机 |
|------|---------|
| `OnInitialize()` | Feature 创建后 |
| `OnBind()` | 绑定客户端连接 |
| `OnAppExit()` / `OnFeatureExit()` | 应用或 Feature 退出 |
| `OnAppInstall()` / `OnAppUninstall()` / `OnAppUpdate()` | 应用生命周期 |
| `OnUserChange()` | OS 用户切换 |
| `Online()` / `Offline()` | 设备上下线 |
| `OnReady()` | 系统就绪 |
| `OnSessionReady()` | 会话就绪 |
| `OnScreenUnlocked()` | 屏幕解锁 |

### IPC 桥接

`FeatureStubImpl` 继承 `IRemoteStub<FeatureStub>`，持有 Feature 实现的共享指针，将 IPC 请求委托给实际 Feature 处理。

### StaticActs 机制

`StaticActs` 是独立于 Feature 的无 IPC 后台动作，使用 `FeatureSystem::RegisterStaticActs()` 注册，主服务在 `LoadFeatures()` 中为其绑定线程池。系统事件直接调用 StaticActs，不经过 IPC。

## Framework 层公共接口

Framework 层使用 `public_configs` 导出的关键接口模块：

| 子目录 | 关键接口 | 职责 |
|--------|---------|------|
| `feature/` | `FeatureSystem`, `StaticActs` | 特性注册与生命周期 |
| `store/` | `GeneralStore`, `AutoCache`, `Cursor` | 通用存储抽象与缓存 |
| `account/` | `AccountDelegate` | 账户事件 |
| `device_manager/` | `DeviceManagerDelegate` | 设备管理 |
| `network/` | `NetworkDelegate` | 网络状态 |
| `cloud/` | `CloudDB`, `CloudServer`, `AssetLoader`, `SchemaMeta` | 云同步抽象 |
| `metadata/` | `MetaDataManager`, `StoreMetaData` 等 | 元数据管理 |
| `dfx/` | `Reporter`, `FaultReporter`, `BehaviourReporter` | 可观测性 |
| `checker/` | `CheckerManager` | 权限校验 |
| `eventcenter/` | `EventCenter`, `Event` | 事件中心 |
| `serializable/` | `Serializable` | 序列化基类 |
| `crypto/` | `CryptoManager` | 加密管理 |
| `snapshot/` | `Snapshot`, `BindEvent` | 资产传输状态机 |
| `sync_mgr/` | `SyncMgr` | 同步管理 |
| `error/` | `GeneralError` | 统一错误码 |
| `directory/` | `DirectoryManager` | 目录与路径管理 |
| `backuprule/` | `BackupRuleManager` | 备份规则 |
| `flow_control_manager/` | `FlowControlManager` | 流控 |
| `dump/` | `DumpManager` | 诊断转储 |

## Feature 开关

所有 Feature 开关在 `datamgr_service.gni` 中定义：

```gni
datamgr_service_cloud = true
datamgr_service_rdb = true
datamgr_service_kvdb = true
datamgr_service_object = true
datamgr_service_data_share = true
datamgr_service_udmf = false      # 默认关闭
datamgr_service_distributed = true
```

修改 Feature 开关默认值前必须先问人。

## 事件驱动架构

系统使用 `EventCenter` 进行松耦合通信。关键事件类型：

| 事件 | 用途 |
|------|------|
| `CloudEvent::CLOUD_SYNC` | 云同步 |
| `CloudEvent::LOCAL_CHANGE` | 本地数据变更 |
| `CloudEvent::GET_SCHEMA` | 获取 Schema |
| `CloudEvent::CLEAN_DATA` | 清理数据 |
| `CloudEvent::CLOUD_SHARE` | 数据共享 |
| `MatrixEvent::MATRIX_ONLINE` | 设备矩阵上线 |
| `MatrixEvent::MATRIX_BROADCAST` | 设备矩阵广播 |
| `BindEvent::BIND_SNAPSHOT` | 资产绑定 |
| `BindEvent::COMPENSATE_SYNC` | 补偿同步 |

## 架构不变量

1. Framework 层不直接调用系统服务，只定义接口
2. App 层不直接依赖 Feature 实现
3. Service 层各 Feature 之间不相互依赖
4. 所有 Delegate 接口遵循依赖注入模式
5. 异步操作使用 ExecutorPool，不启动独立线程
6. 序列化使用 Serializable，不引入外部依赖
