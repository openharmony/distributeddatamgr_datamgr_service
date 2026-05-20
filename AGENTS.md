# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在此代码库中工作时提供指导。

## 项目概述

本项目是 OpenHarmony 的**分布式数据管理服务**（distributeddatamgr_datamgr_service），是 OpenHarmony 分布式数据管理子系统的核心实现。

**核心架构特征**：采用**客户端存储、服务端管理**的分离架构
- **客户端（应用进程）**：负责实际数据存储和访问，数据存储在应用的本地数据库中
- **服务端（本服务）**：负责元数据管理、数据同步协调、静默访问、权限管理、备份恢复等管理功能

**五大核心能力**：
1. **KV/RDB 跨设备同步**：设备间自动数据同步和一致性保障
2. **分布式对象持久化**：分布式对象的完整生命周期管理
3. **跨应用数据静默共享**：无需启动提供方进程即可访问数据
4. **设备-云端同步**：将数据库数据同步到云端
5. **统一数据管理框架（UDMF）**：系统级统一数据管理标准

**数据隔离**：使用（用户、应用、数据库）三元组进行严格数据隔离

## 架构概述

**三层架构**：
1. **应用层**（`app/`）：服务启动入口，Feature 分发，事件分发
2. **服务层**（`service/`）：业务逻辑实现、同步协调、权限管理（包含适配器）
3. **框架层**（`framework/`）：抽象接口定义、依赖注入容器、通用能力管理

**关键设计原则**：
- **适配器属于服务层**：物理位置在 `adapter/` 目录，构建时链接到服务层
- **依赖注入**：框架层定义接口 → 适配器实现接口并调用系统服务 → 注册到框架层 → 服务层使用
- **Feature 架构**：每个服务注册为 Feature 到 FeatureSystem，应用层通过 FeatureSystem 分发

**详细文档**：详见 docs/agent 目录中的对应文档：
- 架构全景图：[docs/agent/architecture-map.md](docs/agent/architecture-map.md)
- 同步域：[docs/agent/domain-sync.md](docs/agent/domain-sync.md)
- 云端域：[docs/agent/domain-cloud.md](docs/agent/domain-cloud.md)
- 数据共享域：[docs/agent/domain-data-share.md](docs/agent/domain-data-share.md)
- UDMF 域：[docs/agent/domain-udmf.md](docs/agent/domain-udmf.md)
- 元数据域：[docs/agent/domain-metadata.md](docs/agent/domain-metadata.md)
- 安全边界：[docs/agent/security-boundary.md](docs/agent/security-boundary.md)
- API 兼容性：[docs/agent/api-compatibility.md](docs/agent/api-compatibility.md)
- DFX 指南：[docs/agent/dfx-guidelines.md](docs/agent/dfx-guidelines.md)

## 关键概念

### 1. GeneralStore（服务端数据库句柄）
- **服务端数据库访问的抽象句柄**，不是客户端抽象
- 为 Feature（端端同步、端云同步等）提供在服务进程中访问数据库的能力
- **主要方法**：Insert/Replace/Update/Delete、Query、Sync、Execute、Watch/Unwatch
- 通过 AutoCache 统一获取和管理

### 2. AutoCache（数据库句柄管理）
- **服务端数据库句柄管理类**，提供缓存和生命周期管理
- **获取句柄**：通过 `GetStore()` 或 `GetDBStore()` 获取数据库句柄
- **句柄回收**：自动垃圾回收机制（可配置间隔），及时释放未使用句柄
- **观察者管理**：支持设置 Watcher 监听数据变化
- **重要**：不要长期持有从 AutoCache 获取的句柄，使用后尽快释放

**使用示例**：
```cpp
auto &cache = AutoCache::GetInstance();
auto [status, store] = cache.GetDBStore(metaData, watchers);
store->Insert("table_name", std::move(value));
// 句柄将被自动回收，不要长期持有
```

### 3. FeatureSystem（特性管理系统）
- **框架层提供的 Feature 注册和管理系统**
- **Feature 注册**：每个 Feature（RDB、KVDB、Cloud、Object 等）通过 `RegisterCreator()` 注册
- **Feature 获取**：应用层通过 `GetCreator()` 获取并实例化 Feature
- **生命周期管理**：管理 Feature 的初始化、绑定、销毁
- **事件分发**：将系统事件（应用安装/卸载、用户切换、设备上下线等）分发到各 Feature
- **关键约束**：应用层不得直接依赖 Feature 实现，通过 FeatureSystem 统一管理

**Feature 生命周期回调**：
- `OnInitialize()`：Feature 初始化
- `OnBind()`：绑定客户端
- `OnAppExit()` / `OnFeatureExit()`：应用或 Feature 退出
- `OnAppInstall()` / `OnAppUninstall()` / `OnAppUpdate()`：应用生命周期
- `OnUserChange()`：用户切换
- `Online()` / `Offline()` / `OnReady()`：设备上下线
- `OnSessionReady()`：会话就绪
- `OnScreenUnlocked()`：屏幕解锁

### 4. StoreMetaData（数据库元数据）
- 包含**单个数据库**的元数据（appId、storeId、user、deviceId、instanceId、isEncrypt 等）
- 不是管理所有存储类型的系统，仅描述单个数据库的配置和状态
- 用于服务端元数据管理和同步协调

### 5. 客户端-服务端交互模式
- **初始连接**：客户端 → 应用层（FeatureStubImpl） → FeatureSystem → Feature
- **后续交互**：客户端 ←→ Feature（直接交互）
- **客户端死亡**：客户端死亡 → 应用层监听 → FeatureSystem 通知 Feature → 清理资源

### 6. 依赖注入机制
- **框架层定义接口**：如 AccountDelegate、DeviceManagerDelegate
- **适配器实现接口**：实现接口并调用系统服务
- **适配器注册到框架层**：初始化时注册实现
- **服务层使用**：通过框架层接口使用能力，不依赖具体实现

## 强制要求（必须遵守）

### 架构分层
- **框架层不得直接调用系统服务**：框架层仅定义接口，系统服务调用由适配器完成
- **应用层不得直接依赖 Feature 实现**：应用层不感知 Feature 实现
- **服务层 Feature 不得相互依赖**：Feature 实现不得依赖其他 Feature，每个 Feature 可独立配置不编译

### 代码质量
- **遵循依赖注入模式**：服务层通过框架层接口使用能力，不依赖具体实现
- **正确命名空间**：使用正确命名空间（框架层和通用模块使用 `OHOS::DistributedData`）

### 许可证
- **所有源文件必须包含 Apache 2.0 许可证头**：C/C++/H 文件和 Rust 文件格式不同
- **版权年份必须为当前年份**：创建新文件时使用当前年份（如 2026）

### 代码风格
- **遵循 .clang-format 配置**：访问修饰符偏移 -4，函数参数不对齐，短函数/循环不允许单行
- **遵循 rustfmt.toml 配置**：Rust 2021 edition

### 性能
- **异步操作使用执行器，不得启动独立线程**：使用 ExecutorPool 进行异步操作

### 使用细节
- **统一使用 AutoCache 获取数据库句柄**：服务端不得使用客户端打开数据库
- **不要长期持有 AutoCache 获取的句柄**：服务端获取的句柄需及时关闭，不能长期持有
- **统一使用 Serializable 进行序列化和反序列化**：不得使用外部依赖进行序列化和反序列化

### 测试
- **修改和新代码必须有 UT 覆盖**：每次修改必须有对应的测试覆盖
- **新增外部接口必须有 FUZZ 测试覆盖**：外部接口需要 FUZZ 测试覆盖以防止恶意输入
- **禁止直接测试私有方法**：应视为实现细节，通过公共接口覆盖。若无法做到，考虑优化代码可测试性
- **Mock 时使用统一 Mock 类**：需要不同实现的场景可继承统一 Mock 类实现独立逻辑，通用逻辑放 Mock 类中，避免过多重复 Mock 代码
- **测试用例必须包含断言**：每个测试用例必须包含显式断言（EXPECT_XXX、ASSERT_XXX）来验证结果。禁止无断言的测试用例：
  - ❌ **错误**：测试用例仅调用方法，无任何断言
  - ❌ **错误**：所有断言隐藏在辅助方法中，测试用例无直接断言
  - ✅ **正确**：测试用例包含如 `EXPECT_EQ`、`EXPECT_TRUE` 等显式断言
  - ✅ **正确**：辅助方法返回值在测试用例中被断言
  - **原因**：无断言的测试无验证价值，产生虚假信心
- **辅助方法断言要求**：使用辅助方法（fixture 方法、工具函数）时，断言必须在测试用例中，而非辅助方法中：
  - ❌ **错误**：辅助方法内部包含 `EXPECT_EQ(result, expected)`
  - ✅ **正确**：辅助方法返回 `result`，测试用例断言 `EXPECT_EQ(helper(), expected)`
  - ✅ **正确**：辅助方法返回 `bool`，测试用例断言 `EXPECT_TRUE(helper())`
  - **原因**：保持断言在测试用例中可见，便于理解和调试
- **简洁优于封装**：优先编写简单可读的测试代码，而非过度方法封装。像 `CreateAndSubscribeObserver()` 这样的通用模式可接受，但应避免复杂的断言辅助方法：
  - **示例**：ScreenLock 测试中 `CreateAndSubscribeObserver()` 可接受，因为它仅准备数据不含断言
  - **原因**：测试应自文档化，过度封装隐藏测试意图
- **禁止不可能失败的断言**：测试用例不得使用永远通过的断言（如 `EXPECT_TRUE(true)`、`EXPECT_FALSE(false)`）。所有断言必须验证实际行为：
  - ❌ **错误**：`EXPECT_TRUE(true)` — 此断言永远不会失败
  - ❌ **错误**：`EXPECT_FALSE(false)` — 此断言永远不会失败
  - ❌ **错误**：`EXPECT_NO_THROW(statement)` — OpenHarmony 不允许（异常已禁用）
  - ✅ **正确**：`EXPECT_EQ(actual, expected)` — 验证实际值与期望值
  - ✅ **正确**：`EXPECT_NE(ptr, nullptr)` — 验证指针非空
  - ✅ **正确**：调用函数并验证可观察的行为或状态变化
  - **原因**：不可能失败的测试无价值，产生代码质量的虚假信心
- **测试用例作者标注**：所有测试用例必须在测试文档头中包含 `@tc.author: agent`，表示 AI 生成或 AI 修改的测试。适用于：
  - 新创建的测试用例
  - 修改的测试用例（包括重构、优化或增强的测试）
  - AI 代理（如 Claude Code）生成或修改的测试用例

  测试头示例：
  ```cpp
  /**
   * @tc.name: Subscribe001
   * @tc.desc: 使用空观察者订阅应返回而不添加
   * @tc.type: FUNC
   * @tc.require:
   * @tc.author: agent
   */
  ```

## 推荐规则（应当遵守）

### 开发指南
- **优先使用框架层提供的抽象**：如 AccountDelegate、DeviceManagerDelegate、EventCenter
- **使用智能指针管理生命周期**：避免内存泄漏
- **使用执行器时避免同步等待其他异步任务**：防止线程阻塞和死锁
- **避免在异步操作中捕获 this**：避免 UAF
- **避免跨层直接调用**：上层调用下层，下层通过接口回调
- **提取通用依赖或工具为公共类或方法**：如 Feature 可共享的事件可提取到应用层统一分发，无需每个 Feature 独立感知；应用层和服务层可共享的工具可放在框架层

### 性能优化
- **服务端实现流量控制**：防止 DOS
- **IPC 回调中如不需要返回值则使用异步 IPC**：减少 IPC 线程长时间占用

### 错误处理
- **适配器负责处理系统服务调用异常**：封装并转换为框架层接口
- **使用统一错误码**：参照框架层定义的错误码

### 测试
- **UT 测试应遵循 FIRST 原则**：Fast（快速）、Independent（独立）、Repeatable（可重复）、Self-Validating（自验证）、Timely（及时）
- **测试名称应清晰表达意图**：推荐格式：被测方法_测试场景_预期结果
- **使用"准备-执行-断言"模式组织测试**：测试结果清晰
- **使用 GMock 对象隔离依赖**：优先使用 GMOCK

## 代码示例

### 正确：使用框架层接口

```cpp
// ✅ 正确：通过框架层接口获取能力
auto &delegate = AccountDelegate::GetInstance();
delegate->GetAccount();
```

### 错误：直接调用适配器

```cpp
// ❌ 错误：直接依赖适配器层实现
auto adapter = std::make_shared<AccountAdapterImpl>();
adapter->GetAccount();
```

### 正确：使用 AutoCache

```cpp
// ✅ 正确：通过 AutoCache 获取句柄
auto &cache = AutoCache::GetInstance();
auto [status, store] = cache.GetDBStore(metaData, {});
store->Insert("table_name", std::move(value));
// 句柄将被自动回收，不要长期持有
```

### 错误：长期持有句柄

```cpp
// ❌ 错误：长期持有数据库句柄
class Service {
    std::shared_ptr<GeneralStore> store_; // 不要长期持有
};
```

### 事件系统

```cpp
auto &center = EventCenter::GetInstance();
center.Subscribe(evtId, [](const Event &event) { /* 处理事件 */ });
center.PostEvent(std::make_unique<Event>(evtId));
center.Unsubscribe(evtId);
```

## 构建系统

### 构建工具：GN（Generate Ninja）

项目使用 OpenHarmony 基于 GN 的构建系统，BUILD.gn 文件定义构建目标。

### 快速构建

```bash
# 构建所有数据服务模块
./build.sh --product-name <product> --build-target datamgr_service

# 快速构建（如未修改 gn 文件）
./build.sh --product-name <product> --build-target datamgr_service --fast-rebuild

# 构建所有单元测试
./build.sh --product-name <product> --build-target datamgr_service_test
```

### Feature 开关

在 `datamgr_service.gni` 中控制：
```gni
datamgr_service_cloud = true         # 云端同步支持（默认启用）
datamgr_service_rdb = true           # 关系型数据库支持（默认启用）
datamgr_service_kvdb = true          # 键值数据库支持（默认启用）
datamgr_service_object = true        # 对象存储支持（默认启用）
datamgr_service_data_share = true    # 数据共享支持（默认启用）
datamgr_service_udmf = false         # UDMF 服务（默认禁用）
```

## 目录结构

```
services/distributeddataservice/
├── adapter/                  # 适配器（属于服务层）
│   ├── account/              # 账号系统适配器
│   ├── communicator/         # 通信适配器
│   ├── dfx/                  # 可观测性适配器
│   ├── network/              # 网络适配器
│   ├── screenlock/           # 屏幕锁定适配器
│   └── utils/                # 工具类
├── app/                      # 应用层
│   └── src/                  # 源代码
├── framework/                # 框架层
│   ├── include/              # 公共 API
│   └── <component>/          # 组件实现
├── service/                  # 服务层
│   ├── rdb/                  # RDB Feature
│   ├── kvdb/                 # KVDB Feature
│   ├── cloud/                # 云端 Feature
│   ├── object/               # 对象 Feature
│   ├── data_share/           # 数据共享 Feature
│   ├── udmf/                 # UDMF Feature
│   └── permission/           # 权限检查
└── rust/                     # Rust 扩展
    └── ylong_cloud_extension/
```

## 常见问题

- **AutoCache 句柄泄漏**：不要将 AutoCache 获取的句柄作为成员变量长期持有
- **Feature 未找到**：检查 Feature 是否在 `datamgr_service.gni` 中启用并正确注册
- **构建错误**：确保模块依赖间的 Feature 标志一致
- **循环依赖**：服务层 Feature 不得相互依赖
- **找不到 'log_print.h' 文件**：确保在 BUILD.gn 中添加 `external_deps = [ "kv_store:datamgr_common" ]` 以访问日志宏（ZLOGD、ZLOGE 等）

## 参考文档

- OpenHarmony 架构概述：https://gitcode.com/openharmony/docs/blob/master/zh-cn/readme/分布式数据管理子系统.md
- 应用开发指南：https://gitcode.com/openharmony/docs/tree/master/zh-cn/application-dev/database
- 项目 README：[README.md](README.md) / [README_zh.md](README_zh.md)

## 总结

在此代码库开发时，必须：
1. **理解架构分离**：明确客户端和服务端职责边界
2. **理解三层架构**：应用层、服务层（含适配器）、框架层
3. **理解关键概念**：GeneralStore（服务端句柄）、AutoCache（句柄管理）、FeatureSystem（特性管理）、StoreMetaData（元数据）
4. **遵循依赖注入**：服务层通过框架层接口获取能力
5. **遵循强制要求**：架构分层、代码质量、许可证、代码风格、性能、测试
6. **确保测试覆盖**：修改和新代码必须有 UT 覆盖，外部接口必须有 FUZZ 测试覆盖
