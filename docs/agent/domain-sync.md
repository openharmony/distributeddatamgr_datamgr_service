# 同步领域知识

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。

## 1. 适用范围

本文档适用于以下场景：
- 修改 `service/rdb/`、`service/kvdb/`、`service/object/` 目录下的同步逻辑
- 修改框架层同步相关接口
- 涉及端端同步、数据冲突、水印管理的行为变更

## 2. 同步模型

### 2.1 端端同步

端端同步负责设备之间的数据双向同步，通过 SyncManager 协调。

### 2.2 数据冲突

同步过程中的数据冲突需要通过冲突处理策略解决。

### 2.3 水印管理

水印用于标记同步进度，确保数据一致性。

---

> **TODO**：补充端端同步完整时序图、冲突处理策略、水印机制说明、RDB/KVDB/Object 各模块的同步差异。
