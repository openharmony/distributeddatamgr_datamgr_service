# Common Event Mock库

## 概述

这是adapter层的共享mock库，用于测试common_event相关功能，消除对系统common_event_service的依赖。该mock库提供了与真实系统服务兼容的接口，使测试更加独立和稳定。

## 支持的模块

- **screenlock**: 屏幕锁定/解锁事件
- **power_manager**: 充电/放电事件
- **其他adapter模块**: 可扩展支持更多事件类型

## 架构

### 文件结构
```
mock/
├── README.md                    # 本说明文档
├── mock_common_event.h          # Mock 主要定义
├── mock_common_event.cpp        # Mock 实现
├── common_event_manager.h       # 公共事件管理器 mock（重定向）
├── common_event_subscriber.h    # 公共事件订阅者 mock（重定向）
└── common_event_support.h       # 公共事件支持 mock（重定向）
```

### 设计原理

**核心思想**：创建与真实头文件同名的 mock 文件，当生产代码包含这些头文件时，实际使用的是 mock 实现。

#### 1. 依赖的 Common Event 接口分析

Mock 实现了以下 common_event 接口：

**CommonEventSubscriber** (基类)
```cpp
- 构造函数：CommonEventSubscriber(const CommonEventSubscribeInfo &info)
- 虚函数：OnReceiveEvent(const CommonEventData &event)
```

**CommonEventSubscribeInfo**
```cpp
- 构造函数：CommonEventSubscribeInfo(const MatchingSkills &skills)
```

**MatchingSkills**
```cpp
- 方法：AddEvent(const std::string& event)
```

**CommonEventData**
```cpp
- 方法：GetWant()
```

**Want**
```cpp
- 方法：GetAction(), GetIntParam(key, default)
```

**CommonEventManager** (静态类)
```cpp
- SubscribeCommonEvent(subscriber) → bool
- UnSubscribeCommonEvent(subscriber) → bool
```

**CommonEventSupport**
```cpp
- 常量：COMMON_EVENT_SCREEN_UNLOCKED, COMMON_EVENT_SCREEN_LOCKED
- 常量：COMMON_EVENT_CHARGING, COMMON_EVENT_DISCHARGING
```

#### 2. Mock 实现

所有接口都在 `mock_common_event.h` 中实现，主要特性：

1. **Want 类**：模拟事件数据载体
   - 支持设置/获取 action 和参数

2. **CommonEventData 类**：包装 Want 数据

3. **MatchingSkills 类**：管理订阅的事件列表

4. **CommonEventSubscribeInfo 类**：存储订阅信息

5. **CommonEventSubscriber 基类**：提供虚函数接口

6. **CommonEventManager 类**：静态方法，可控制返回值
   ```cpp
   // 控制订阅/取消订阅的返回结果
   static void SetSubscribeResult(bool result);
   static void SetUnsubscribeResult(bool result);
   static void Reset();
   
   // 发布事件
   static void PublishScreenUnlockEvent(int32_t userId);
   static void PublishScreenLockedEvent(int32_t userId);
   static void PublishChargingEvent(int32_t batteryLevel);
   static void PublishDisChargingEvent(int32_t batteryLevel);
   ```

7. **CommonEventSupport 类**：提供事件常量

#### 3. 头文件重定向

创建了三个与真实头文件同名的文件：
- `common_event_manager.h`
- `common_event_subscriber.h`
- `common_event_support.h`

这些文件都简单地包含 `mock_common_event.h`，实现头文件重定向。

## 使用方法

### 1. 在BUILD.gn中配置

```gn
config("module_private_config") {
  include_dirs = [
    # ... 其他包含路径
    "${data_service_path}/adapter/test/mock",  # 添加mock路径
  ]
}

ohos_unittest("YourTest") {
  sources = [
    "your_test.cpp",
  ]
  
  deps = [
    "${data_service_path}/adapter/test:dist_common_event_mock",  # 使用共享mock库
    # ... 其他依赖
  ]
  
  # 移除系统服务依赖
  # external_deps = [
  #   "common_event_service:cesfwk_innerkits",  # 不再需要
  # ]
}
```

### 2. 在测试中使用

```cpp
#include "mock/mock_common_event.h"  // 包含 mock 定义
#include "your_power_manager.h"   // 包含生产代码（会使用 mock）

// 在测试中控制 mock 行为
OHOS::EventFwk::CommonEventManager::SetSubscribeResult(false);
OHOS::EventFwk::CommonEventManager::PublishChargingEvent(85);
OHOS::EventFwk::CommonEventManager::Reset();
```

## Mock 控制接口

### CommonEventManager 静态方法

#### 控制接口
```cpp
// 设置订阅操作的返回值
static void SetSubscribeResult(bool result);

// 设置取消订阅操作的返回值
static void SetUnsubscribeResult(bool result);

// 重置为默认值（true）
static void Reset();
```

#### 事件发布接口
```cpp
// ScreenLock事件
static void PublishScreenUnlockEvent(int32_t userId);
static void PublishScreenLockedEvent(int32_t userId);

// PowerManager事件
static void PublishChargingEvent(int32_t batteryLevel);
static void PublishDisChargingEvent(int32_t batteryLevel);
```

## 支持的事件类型

### ScreenLock事件

| 事件常量 | 事件名称 | 说明 | 参数 |
|---------|----------|------|------|
| COMMON_EVENT_SCREEN_UNLOCKED | usual.event.SCREEN_UNLOCKED | 屏幕解锁 | userId (int32_t) |
| COMMON_EVENT_SCREEN_LOCKED | usual.event.SCREEN_LOCKED | 屏幕锁定 | userId (int32_t) |

### PowerManager事件

| 事件常量 | 事件名称 | 说明 | 参数 |
|---------|----------|------|------|
| COMMON_EVENT_CHARGING | usual.event.CHARGING | 充电中 | batteryLevel (int32_t) |
| COMMON_EVENT_DISCHARGING | usual.event.DISCHARGING | 放电中 | batteryLevel (int32_t) |

## 测试场景

Mock 支持以下测试场景：

1. **订阅成功/失败**：通过 `SetSubscribeResult()` 控制
2. **取消订阅成功/失败**：通过 `SetUnsubscribeResult()` 控制
3. **事件数据模拟**：通过 Want 和 CommonEventData 类
4. **订阅信息管理**：通过 MatchingSkills 和 CommonEventSubscribeInfo 类
5. **事件发布**：通过 Publish*Event 方法模拟系统事件
6. **并发测试**：支持多线程环境下的测试

## 优势

1. **零依赖**：不依赖 common_event_service 模块
2. **完全可控**：可以精确控制 mock 行为
3. **轻量级**：只实现必要的接口
4. **易维护**：独立的 mock 目录，不影响生产代码
5. **真实模拟**：接口签名与真实实现一致
6. **可扩展**：支持多个adapter模块复用

## 注意事项

1. Mock 只用于单元测试，不影响生产代码
2. Mock 类实现了所有adapter模块需要的通用接口
3. 在测试开始前调用 `Reset()` 重置 mock 状态
4. Mock 文件必须在 include 路径中优先于真实头文件
5. 使用共享mock库时，不需要在本地复制mock文件

## 迁移指南

如果你之前使用的是本地的mock实现（如screenlock/test/mock），迁移到共享mock库：

1. 更新BUILD.gn中的include_dirs指向新的mock路径
2. 移除本地的mock_common_event.cpp从sources中
3. 添加对dist_common_event_mock的deps依赖
4. 验证测试编译和运行

详细的迁移示例请参考screenlock/test/BUILD.gn的修改。
