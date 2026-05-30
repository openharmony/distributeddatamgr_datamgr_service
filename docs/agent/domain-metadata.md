# 元数据领域知识

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。

## 1. 适用范围

本文档适用于以下场景：
- 修改 `framework/include/metadata/` 目录下的定义
- 修改 `framework/include/store/general_store.h` 或 `auto_cache.h`
- 涉及 StoreMetaData、GeneralStore、AutoCache 的变更

## 2. 核心概念

### 2.1 StoreMetaData

- 单数据库元数据描述，不是全局存储管理系统。
- 包含数据库标识、用户、bundle、store 等关键信息。
- 作为数据库句柄查找和路由的依据。

### 2.2 GeneralStore

- 通用存储接口，端云同步通过 GeneralStore::Sync 执行。
- 提供 Sync、Bind、Clean、LockCloudDB 等核心操作。

### 2.3 AutoCache

- 服务端统一的数据库句柄管理器。
- 通过 AutoCache::GetStore 获取数据库句柄。
- 不得长期持有 AutoCache 获取的句柄作为成员变量。
- StoreOption.createRequired 默认为 false。

## 3. 使用规则

- 服务端必须通过 AutoCache 统一获取数据库句柄，不得使用客户端方式打开。
- 使用后尽快释放句柄，不得缓存为成员变量。
- StoreMetaData 变更影响数据库路由，需确认全局一致性。

---

> **TODO**：补充 StoreMetaData 字段完整说明、AutoCache 生命周期管理、句柄泄漏排查方法。
