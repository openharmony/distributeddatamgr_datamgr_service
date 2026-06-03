# KVDB 同步领域知识

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。

## 1. 适用范围

本文档适用于以下场景：
- 修改 `services/distributeddataservice/service/kvdb/` 目录下的同步逻辑
- 修改框架层 KVDB 同步相关接口
- 涉及 KVDB 端端同步、数据冲突、水印管理的行为变更

## 2. 同步模型

### 2.1 端端同步

端端同步负责设备之间键值数据的双向同步。

### 2.2 数据冲突

同步过程中的数据冲突需要通过冲突处理策略解决。

### 2.3 水印管理

水印用于标记同步进度，确保数据一致性。

---

> **TODO**：补充 KVDB 同步完整时序图、冲突处理策略、水印机制说明。
