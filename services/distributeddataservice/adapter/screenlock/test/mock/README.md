# Common Event Mock

## 概述
这个目录包含了用于替代 OpenHarmony 公共事件服务（Common Event Service）的 mock 实现，使 ScreenLock 单元测试能够独立运行，不依赖系统服务。

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

ScreenLock 依赖以下 common_event 接口：

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
   ```

7. **CommonEventSupport 类**：提供事件常量

#### 3. 头文件重定向

创建了三个与真实头文件同名的文件：
- `common_event_manager.h`
- `common_event_subscriber.h`
- `common_event_support.h`

这些文件都简单地包含 `mock_common_event.h`，实现头文件重定向。

## 使用方法

### 在 BUILD.gn 中配置

```gn
config("module_private_config") {
  include_dirs = [
    # ... 其他包含路径
    "//foundation/distributeddatamgr/datamgr_service/.../test/mock",
  ]
}

ohos_unittest("ScreenLockTest") {
  sources = [
    "screen_lock_test.cpp",
    "mock/mock_common_event.cpp"  # 包含 mock 实现
  ]
  # ... 其他配置
}
```

### 在测试中使用

```cpp
#include "mock/mock_common_event.h"  // 包含 mock 定义
#include "screenlock/screen_lock.h"   // 包含生产代码（会使用 mock）

// 在测试中控制 mock 行为
OHOS::EventFwk::CommonEventManager::SetSubscribeResult(false);
OHOS::EventFwk::CommonEventManager::SetUnsubscribeResult(true);
```

## Mock 控制接口

### CommonEventManager 静态方法

```cpp
// 设置订阅操作的返回值
static void SetSubscribeResult(bool result);

// 设置取消订阅操作的返回值
static void SetUnsubscribeResult(bool result);

// 重置为默认值（true）
static void Reset();
```

## 测试场景

Mock 支持以下测试场景：

1. **订阅成功/失败**：通过 `SetSubscribeResult()` 控制
2. **取消订阅成功/失败**：通过 `SetUnsubscribeResult()` 控制
3. **事件数据模拟**：通过 Want 和 CommonEventData 类
4. **订阅信息管理**：通过 MatchingSkills 和 CommonEventSubscribeInfo 类

## 优势

1. **零依赖**：不依赖 common_event_service 模块
2. **完全可控**：可以精确控制 mock 行为
3. **轻量级**：只实现必要的接口
4. **易维护**：独立的 mock 目录，不影响生产代码
5. **真实模拟**：接口签名与真实实现一致

## 注意事项

1. Mock 只用于单元测试，不影响生产代码
2. Mock 类只实现了 ScreenLock 需要的接口
3. 在测试开始前调用 `Reset()` 重置 mock 状态
4. Mock 文件必须在 include 路径中优先于真实头文件
