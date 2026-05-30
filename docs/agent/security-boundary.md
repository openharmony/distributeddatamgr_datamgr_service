# 安全边界规范

> 本文档为 Agent 知识路由目标，由 AGENTS.md 的 task-based / path-based / vocabulary-based routing 触发阅读。

## 1. 适用范围

本文档适用于以下场景：
- 修改权限校验逻辑
- 修改云开关或认证状态判断
- 修改 `app/distributed_data.cfg` 中的权限声明
- 修改跨应用数据访问控制

## 2. 权限要求

| 权限 | 说明 | 类型 |
|------|------|------|
| ohos.permission.CLOUDFILE_SYNC | 云文件同步权限 | 普通 |
| ohos.permission.GET_NETWORK_INFO | 获取网络信息权限 | 普通 |
| ohos.permission.MONITOR_DEVICE_NETWORK_STATE | 监控设备网络状态权限 | ACL |
| ohos.permission.USE_CLOUD_DRIVE_SERVICE | 使用云盘服务权限 | 普通 |
| ohos.permission.PUBLISH_SYSTEM_COMMON_EVENT | 发布系统公共事件 | 普通 |
| ohos.permission.GET_BUNDLE_INFO | 获取应用信息 | 普通 |
| ohos.permission.GET_BUNDLE_INFO_PRIVILEGED | 获取应用信息（特权） | 系统 |

## 3. 安全规则

### 3.1 权限校验不变量

- 权限校验必须在能力入口点执行，不得在内部方法中绕过。
- 云开关状态是同步流程的前置条件，所有同步操作必须检查开关状态。
- 认证状态与信任状态不可混淆：已发现的设备不等于已认证或已信任的设备。

### 3.2 禁止事项

- 不得绕过权限校验或删除权限检查以通过测试。
- 不得降低权限等级要求。
- 不得在日志中输出敏感信息（Token、密钥、用户凭证等）。

### 3.3 变更审批

以下变更必须经安全评审：
- 新增或删除权限声明。
- 变更权限校验逻辑。
- 变更云开关或信任模型。
- 变更跨应用数据访问控制策略。

---

> **TODO**：补充威胁模型分析和历史安全事件案例。
