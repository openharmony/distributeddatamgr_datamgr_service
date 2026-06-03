# 分布式对象领域知识

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。

## 1. 适用范围

本文档适用于以下场景：
- 修改 `services/distributeddataservice/service/object/` 目录下的逻辑
- 修改框架层分布式对象相关接口
- 涉及对象生命周期管理、对象同步的行为变更

## 2. 核心概念

### 2.1 分布式对象

分布式对象支持跨设备对象状态同步，通过对象 sessionId 管理生命周期。

### 2.2 对象同步

对象状态变更自动同步到同 sessionId 的远端对象。

### 2.3 生命周期管理

对象的创建、销毁、sessionId 变更影响同步行为。

---

> **TODO**：补充分布式对象时序图、生命周期状态机、同步机制说明。
