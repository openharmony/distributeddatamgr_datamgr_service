# API 兼容性规范

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。

## 1. 适用范围

本文档适用于以下场景：
- 修改 `interfaces/` 目录下的公共 API 定义
- 修改 IPC 接口签名（CloudServiceStub Handler 编号、参数顺序、错误码）
- 修改跨模块依赖的类型定义（如 `cloud_types.h`）

## 2. 兼容性规则

### 2.1 公共 API 不变量

- 公共 API 签名不得随意变更，参数顺序、类型、返回值语义必须保持稳定。
- 错误码值不得重新分配，新增错误码只能追加。
- 权限声明变更属于兼容性敏感变更，必须经审批。

### 2.2 IPC 接口规则

- CloudServiceStub Handler 编号不得重新分配，新增只能追加。
- IPC 参数顺序变更视为破坏性变更。
- 回调语义变更（如 OnComplete 回调参数含义）影响应用层，必须明确标注。

### 2.3 跨模块类型定义

- `cloud_types.h` 中的 Role/Confirmation/Privilege/Participant/Strategy 等类型由 `distributeddatamgr_relational_store` 定义。
- 变更这些类型需同步确认 relational_store 侧的兼容性。

## 3. 兼容性检查方法

- 检查公共接口签名是否与上一版本一致。
- 检查 IPC Handler 编号是否只增不减。
- 检查错误码是否只追加不修改。
- 检查 `cloud_types.h` 类型定义是否与 relational_store 侧对齐。

---

> **TODO**：补充具体的兼容性检查命令和历史兼容性问题案例。
