# 架构图

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。

## 1. 三层架构

```
┌─────────────────────────────────────────────────┐
│ 应用层 (app/)                                    │
│   服务启动入口、FeatureSystem 分发、事件分发       │
├─────────────────────────────────────────────────┤
│ 服务层 (service/) + 适配器 (adapter/)             │
│   业务逻辑实现、同步协调、权限管理                  │
├─────────────────────────────────────────────────┤
│ 框架层 (framework/)                              │
│   抽象接口定义、依赖注入容器、通用能力管理           │
└─────────────────────────────────────────────────┘
```

**关键边界**：框架层仅定义接口，系统服务调用由适配器完成。应用层通过 FeatureSystem 分发，不感知功能模块实现。

## 2. 功能模块关系

```
app/ (FeatureSystem)
  ├── service/rdb/     ← RDB 功能模块
  ├── service/kvdb/    ← KVDB 功能模块
  ├── service/cloud/   ← 端云功能模块
  ├── service/data_share/ ← 数据共享模块
  ├── service/object/  ← 分布式对象模块
  ├── service/udmf/    ← UDMF 模块
  └── service/backup/  ← 备份恢复模块

framework/ (接口 + 容器)
  ├── include/cloud/   ← 端云框架接口
  ├── include/store/   ← 存储框架接口
  └── include/metadata/ ← 元数据接口

adapter/ (系统服务适配)
  └── 各系统服务的依赖注入实现
```

**模块独立性规则**：功能模块之间不得相互依赖，每个模块支持配置和排除编译。

## 3. 关键设计模式

### 3.1 FeatureSystem

- 功能模块注册和生命周期管理。
- 应用层通过 FeatureSystem 分发请求，不直接依赖功能模块实现类。
- 不得直接跨模块依赖。

### 3.2 依赖注入 / Delegate

- 框架层接口通过 Delegate 模式实现依赖注入。
- 服务层不得直接依赖适配器实现，必须通过框架层接口。

### 3.3 ExecutorPool

- 异步操作统一使用 ExecutorPool，不得启动独立线程。
- 异步回调中不得捕获 this 指针（避免 UAF）。

### 3.4 Serializable

- 序列化和反序列化统一使用 Serializable 机制。
- 不得引入外部序列化依赖。

### 3.5 AutoCache

- 服务端通过 AutoCache 统一获取数据库句柄。
- 不得使用客户端方式打开数据库。
- 不得长期持有 AutoCache 获取的句柄，使用后尽快释放。

---

> **TODO**：补充各模块间完整的调用时序图和组件生命周期图。
