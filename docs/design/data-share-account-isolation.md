# DataShare 账号隔离数据访问方案设计

## 1. 背景与问题

### 1.1 现状

DataShare 静默访问当前按 **OS 用户（userId）** 隔离数据：

- userId=100 的应用访问 userId=100 下数据提供方的数据
- userId=200 的应用访问 userId=200 下数据提供方的数据
- 数据天然根据 OS 用户系统隔离

关键机制：
- `visitedUserId`：从调用方 tokenId 或 URI 参数 `?user=X` 获取，决定访问哪个 OS 用户的数据
- `StoreMetaData` 元数据存储键：`{deviceId}:{user}:default:{bundleName}:{storeId}:{instanceId}:{dataDir}`，其中 `instanceId = appIndex`（分身ID）；查询键（`StoreMetaMapping::GetKey`）不含 dataDir
- `AccessCrossMode`：`USER_SHARED`（单例共享）/ `USER_SINGLE`（按用户隔离表名后缀 `_userId`）

### 1.2 新场景

车机等单用户多账号产品：
- 系统不区分多 OS 用户，但可登录不同账号（中控屏账号A，后排账号B）
- 同一应用存在同名数据库，数据库路径中有 **账号ID（accountId）** 区分
- 应用分身与账号绑定：每个账号对应一个应用分身（appIndex）

### 1.3 设备类型约束（关键）

**本需求仅在车机设备上生效，其他设备（手机、平板、手表、PC 等）必须完全隔离此功能。**

原因：
- 车机场景：单 OS 用户多账号共存，需要账号级数据隔离
- 手机/平板/手表：多 OS 用户隔离机制已满足需求，账号隔离不适用且可能引入数据混乱
- PC：不支持跨用户访问（已有 `PLATFORM_PC` 编译隔离）

隔离要求：
- **编译隔离**：非车机产品配置下，账号隔离相关代码不参与编译
- **运行隔离**：即使代码被编译（如通用二进制），运行时在非车机设备上自动降级为原有逻辑
- **接口隔离**：非车机设备上 URI 中的 `accountId` 参数被忽略，`accountIsolation` 配置无效

### 1.3 核心概念映射

| 原概念 | 新概念 | 说明 |
|--------|--------|------|
| userId (osAccountId) | userId (osAccountId) | OS 用户ID，不变 |
| appIndex (instIndex) | appIndex (instIndex) | 应用分身ID，不变 |
| 无 | accountId (subProfileId) | 账号ID，新增 |
| 无 | accountIsolation | 配置项：提供方是否按账号隔离数据 |

**关键映射关系**：`accountId (subProfileId) ↔ appIndex`，通过账号子系统接口互相转换。

---

## 2. 场景分析

### 场景一：三方应用（有分身）访问系统应用数据

```
访问方: 三方应用（appIndex != 0）
提供方: 系统应用（数据按账号隔离）

流程:
1. 调用方通过分身ID从账号子系统获取所属 accountId
   → AccountDelegate::GetSubProfileIdByToken(callerTokenId)
2. 忽略 URI 中的 accountId 参数
3. 服务端根据 accountId 拼接数据库路径，访问对应数据
```

### 场景二：系统应用访问系统应用数据

```
访问方: 系统应用（TOKEN_NATIVE / TOKEN_SHELL）
提供方: 系统应用（数据按账号隔离）

流程:
1. 从 URI 参数获取 accountId
   URI: datashareproxy://xxx/xxx?accountId=123
   → URIUtils::GetAccountIdFromProxyURI(uri, accountIdStr)
   若未指定 accountId，则取当前前台账号：
   → AccountDelegate::GetForegroundSubProfileId(visitedUserId)
2. 根据 accountId 拼接数据库路径，访问对应数据
```

### 场景三：系统应用访问三方应用数据

```
访问方: 系统应用（TOKEN_NATIVE / TOKEN_SHELL）
提供方: 三方应用（有分身，数据按账号隔离）

流程:
1. 从 URI 参数获取 accountId
2. 将 accountId 转换为提供方应用的分身ID（appIndex）
   → AccountDelegate::GetAppIndexBySubProfileId(osAccountId, subProfileId)
3. 根据分身ID查询元数据，获取数据库路径
4. 访问数据库
```

### 补充场景：无分身三方应用

```
访问方: 三方应用（appIndex == 0）
提供方: 任意应用

规则:
- 忽略 URI 中的 accountId 参数
- 只允许访问前台账号对应的数据
- 前台 accountId = AccountDelegate::GetForegroundSubProfileId(osAccountId)
```

---

## 3. 外部依赖

账号子系统的 `OsAccountSubProfileClient` 已上线，头文件路径：
`base/account/os_account/interfaces/innerkits/os_account_subspace/native/include/os_account_subprofile_client.h`

> 注：该客户端受编译宏 `ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE` 控制，车机产品需确保该宏已启用；通过 `OsAccountSubProfileClient::GetInstance()` 单例访问。

### 3.1 本方案依赖接口

```cpp
class OsAccountSubProfileClient {
public:
    static OsAccountSubProfileClient &GetInstance();

    // 1. 通过 tokenId 获取 subProfileId（不含 userId 入参）
    ErrCode GetOsAccountSubProfileId(uint32_t tokenId, int32_t &subProfileId);

    // 2. 获取指定 OS 用户当前前台的 subProfileId（新增接口）
    ErrCode GetOsAccountForegroundSubProfileId(int32_t osAccountId, int32_t &subProfileId);

    // 3. 通过 OS用户ID + 分身ID 获取 subProfileId
    ErrCode GetOsAccountSubProfileId(int32_t osAccountLocalId, int32_t appIndex, int32_t &subProfileId);

    // 4. 通过 OS用户ID + subProfileId 获取分身信息（subspaceResult.index 即 appIndex）
    ErrCode GetOsAccountSubProfile(int32_t osAccountId, int32_t subProfileId,
        OsAccountSubspaceResult &subspaceResult, OhosAccountInfo &distributedInfo);
};
```

### 3.2 AccountDelegate 接口映射

| AccountDelegate 接口 | 账号子系统接口 | 用途 |
|---------------------|---------------|------|
| `GetSubProfileIdByToken(tokenId, &subProfileId)` | `GetOsAccountSubProfileId(tokenId, &subProfileId)` | 有分身三方应用按 tokenId 取账号 |
| `GetForegroundSubProfileId(osAccountId, &subProfileId)` | `GetOsAccountForegroundSubProfileId(osAccountId, &subProfileId)` | 系统应用未指定 accountId / 无分身应用取前台账号 |
| `GetSubProfileIdByAppIndex(osAccountId, appIndex, &subProfileId)` | `GetOsAccountSubProfileId(osAccountLocalId, appIndex, &subProfileId)` | 按 appIndex 取账号（备用） |
| `GetAppIndexBySubProfileId(osAccountId, subProfileId, &appIndex)` | `GetOsAccountSubProfile(osAccountId, subProfileId, subspaceResult, distributedInfo)` 取 `.index` | 提供方有分身时 accountId → appIndex 转换 |

### 3.3 术语对照

| 本方案术语 | 账号子系统术语 | 说明 |
|------------|---------------|------|
| accountId | subProfileId | 账号ID |
| userId | osAccountId / osAccountLocalId | OS 用户ID（现有概念） |
| appIndex | subspaceResult.index | 分身ID |

### 3.4 依赖状态

- **当前状态**：API 已上线（`OsAccountSubProfileClient`）
- **适配策略**：`AccountDelegateNormalImpl` 直接调用 `OsAccountSubProfileClient::GetInstance()`；`AccountDelegateDefaultImpl` 保持 Stub 返回错误码（无 account 部件时）

---

## 4. 架构设计

### 4.1 设计原则

1. **复用现有元数据维度**：有分身应用复用 `instanceId` 维度，无分身应用复用 `dataDir` 维度查找元数据，无需修改持久化数据结构
2. **遵循框架层接口定义 → 适配器层实现**的分层约束：新增账号接口通过 `AccountDelegate` 适配
3. **功能模块间不相互依赖**：账号隔离逻辑仅在 data_share 模块内
4. **配置驱动**：提供方通过 `module.json5` 声明是否按账号隔离数据
5. **车机设备专属**：双重隔离（编译 + 运行），非车机设备完全不执行账号隔离逻辑

### 4.2 设备类型门控机制（双层隔离）

**约束**：账号隔离功能仅在车机设备生效，其他设备必须完全隔离。

#### 4.2.1 编译隔离

**`datamgr_service.gni`** — 新增编译开关：

```gn
declare_args() {
  # ... 现有参数 ...

  # 账号隔离功能开关，仅车机产品配置启用
  datamgr_service_account_isolation = false
}
```

车机产品配置文件（如 `vendor/xxx/car/productconfig.json` 或产品级 gn 配置）中显式设置：
```gn
datamgr_service_account_isolation = true
```

非车机产品不设置此参数，默认 `false`。

**`service/data_share/BUILD.gn`** — 条件编译：

```gn
config("module_public_config") {
  # ... 现有配置 ...
  if (datamgr_service_account_isolation) {
    defines += [ "ACCOUNT_ISOLATION_ENABLED" ]
  }
}
```

所有账号隔离相关代码块使用 `#ifdef ACCOUNT_ISOLATION_ENABLED` 包裹：
- 访问方身份判断（DataProviderConfig 构造函数新增部分）
- 提供方身份识别 ResolveProviderAppIndex（ExecuteEx 新增部分）
- GetSilentProxyStatus 提供方身份识别
- LoadConfigCommonStrategy accountId 解析

**非车机编译效果**：这些代码块完全不参与编译，二进制中不存在账号隔离逻辑。

#### 4.2.2 运行隔离

即使编译时 `ACCOUNT_ISOLATION_ENABLED` 被定义（如通用二进制分发），运行时仍需验证设备类型。

**`service/data_share/common/common_utils.h`** — 新增运行时设备类型判断：

```cpp
bool IsCarDevice();
```

**`service/data_share/common/common_utils.cpp`** — 实现：

```cpp
bool IsCarDevice()
{
    using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
    auto localDevice = DmAdapter::GetInstance().GetLocalDevice();
    uint32_t localDeviceType = DmAdapter::GetInstance().GetDeviceTypeByUuid(localDevice.uuid);
    return localDeviceType == DmAdapter::DmDeviceType::DEVICE_TYPE_CAR;
}
```

**使用方式**：在所有账号隔离逻辑入口处前置设备类型检查：

```cpp
#ifdef ACCOUNT_ISOLATION_ENABLED
    if (!IsCarDevice()) {
        // 非车机设备：跳过账号隔离逻辑，走原有流程
        ZLOGD("account isolation skipped on non-car device");
    } else {
        // 车机设备：执行账号隔离逻辑
        // ... accountId 解析、转换等 ...
    }
#endif
```

#### 4.2.3 双层隔离汇总

| 阶段 | 车机设备 | 非车机设备 |
|------|---------|-----------|
| 编译 | `ACCOUNT_ISOLATION_ENABLED` 已定义 | `ACCOUNT_ISOLATION_ENABLED` 未定义，相关代码不编译 |
| 运行（车机二进制） | `IsCarDevice()=true`，执行账号隔离逻辑 | N/A（车机专用二进制不会运行在非车机设备） |
| 运行（通用二进制） | `IsCarDevice()=true`，执行账号隔离逻辑 | `IsCarDevice()=false`，跳过账号隔离逻辑，走原有流程 |

**关键保证**：非车机设备上，无论编译还是运行层面，账号隔离逻辑都不生效。

#### 4.2.4 设备类型判断位置

`DeviceManagerAdapter` 已提供 `GetDeviceTypeByUuid()` 方法（`device_manager_adapter.h:71`），返回 `DmDeviceType` 枚举值。`DEVICE_TYPE_CAR = 0x83` 已定义。

关键路径：
- `DeviceManagerAdapter::GetInstance().GetLocalDevice()` → 获取本机 uuid
- `DeviceManagerAdapter::GetInstance().GetDeviceTypeByUuid(uuid)` → 返回设备类型 uint32_t
- 现有代码已有设备类型判断先例：`rdb_service_impl.cpp:1591` 使用相同机制判断 `IsSupportAutoSync`

### 4.3 数据流

```
请求进入 (Query/Insert/Update/Delete)
  │
  ├─ ★ 设备类型门控 ─────────────── IsCarDevice() && ACCOUNT_ISOLATION_ENABLED
  │   ├─ 非车机/未编译: 跳过全部账号隔离逻辑，走原有流程
  │   └─ 车机: 进入账号隔离流程
  │
  ├─ URI 解析 ───────────────────── URIUtils::GetAccountIdFromProxyURI(uri)
  │
  ├─ 静默代理检查 ───────────────── GetSilentProxyStatus (accountId→appIndex→calledTokenId)
  │
  ├─ 访问方身份判断 ─────────────── ResolveAccessorAccountId()
  │   ├─ delegate 判空校验（空则跳过）
  │   ├─ 系统应用: URI accountId / 未指定则 GetForegroundSubProfileId(visitedUserId)
  │   ├─ 有分身三方: GetSubProfileIdByToken(callerTokenId)
  │   └─ 无分身应用: GetForegroundSubProfileId(visitedUserId)
  │
  ├─ 提供方配置解析 ─────────────── DataProviderConfig::GetProviderInfo()
  │   └─ accountIsolation 配置传递到 ProviderInfo
  │
  ├─ 提供方身份判断 ─────────────── ResolveProviderAppIndex()
  │   ├─ 有分身: GetAppIndexBySubProfileId(visitedUserId, accountId) → 覆盖 appIndex
  │   ├─ 无分身 + accountIsolation=true: accountId+storeName 拼接 dataDir → queryByPath=true
  │   └─ 无分身 + accountIsolation=false: 不处理（原 storeName 查询）
  │
  ├─ 权限验证 ────────────────────── VerifyAcrossAccountsPermission (复用)
  │   ├─ CheckAllowList
  │   ├─ VerifyPermission
  │
  ├─ 元数据查找 ─────────────────── QueryMetaData
  │   ├─ 有分身: GetKey()(含 instanceId=appIndex)
  │   ├─ 无分身+accountIsolation: GetStoreMetaKeyWithPath(dataDir)
  │   └─ 无分身+非隔离: GetKey()(instanceId=0)
  │
  └─ 数据库访问 ──────────────────── DBDelegate → 返回结果
```

### 4.4 accountId → 元数据查找链路

按提供方是否有分身分两路：

**有分身应用**（accountId → appIndex → instanceId 维度）：
```
accountId (subProfileId)
  │ AccountDelegate::GetAppIndexBySubProfileId(visitedUserId, accountId)
  ↓
providerAppIndex → 覆盖 providerInfo.appIndex
  │ MetaDataManager::LoadMeta(StoreMetaMapping::GetKey(), storeMetaMapping)
  │   (查询键含 instanceId=appIndex，不含 dataDir，按前缀匹配存储条目)
  ↓
StoreMetaData.dataDir → DB路径
```

**无分身应用 + accountIsolation=true**（accountId → dataDir 维度）：
```
accountId (subProfileId) + storeName
  │ ConstructAccountDataDir(accountId, storeName)
  ↓
dataDir (数据库路径，含 accountId)
  │ MetaDataManager::LoadMeta(StoreMetaMapping::GetStoreMetaKeyWithPath(dataDir), storeMetaMapping)
  │   (查询键含 dataDir，精确匹配存储条目)
  ↓
StoreMetaData.dataDir → DB路径
```

> 无分身应用 + accountIsolation=false：不涉及 accountId，直接 `LoadMeta(StoreMetaMapping::GetKey())`（instanceId=0）走原有逻辑。

---

## 5. 变更详细设计

### 5.1 AccountDelegate 适配器层

#### 5.1.1 框架层接口 — `framework/include/account/account_delegate.h`

新增 4 个纯虚方法：

```cpp
// 通过调用方 tokenId 获取其所属账号的 subProfileId（不含 userId 入参）
// 对应账号子系统: GetOsAccountSubProfileId(uint32_t tokenId, int32_t &subProfileId)
virtual int32_t GetSubProfileIdByToken(uint32_t tokenId, int32_t &subProfileId) const = 0;

// 获取指定 OS 用户当前前台的 subProfileId（系统应用未指定 accountId / 无分身应用使用）
// 对应账号子系统: GetOsAccountForegroundSubProfileId(int32_t osAccountId, int32_t &subProfileId)
virtual int32_t GetForegroundSubProfileId(int32_t osAccountId, int32_t &subProfileId) const = 0;

// 通过 OS用户ID + 分身ID 获取对应的 subProfileId
// 对应账号子系统: GetOsAccountSubProfileId(int32_t osAccountLocalId, int32_t appIndex, int32_t &subProfileId)
virtual int32_t GetSubProfileIdByAppIndex(int32_t osAccountId, int32_t appIndex, int32_t &subProfileId) const = 0;

// 通过 OS用户ID + subProfileId 获取对应的分身ID（appIndex）
// 对应账号子系统: GetOsAccountSubProfile(osAccountId, subProfileId, subspaceResult, distributedInfo)
// 返回 subspaceResult.index 作为 appIndex
virtual int32_t GetAppIndexBySubProfileId(int32_t osAccountId, int32_t subProfileId, int32_t &appIndex) const = 0;
```

#### 5.1.2 正式实现 — `adapter/account/src/account_delegate_normal_impl.h/.cpp`

新增 4 个方法声明和实现。外部 API 已上线，直接调用 `OsAccountSubProfileClient`：

```cpp
// account_delegate_normal_impl.h 新增:
int32_t GetSubProfileIdByToken(uint32_t tokenId, int32_t &subProfileId) const override;
int32_t GetForegroundSubProfileId(int32_t osAccountId, int32_t &subProfileId) const override;
int32_t GetSubProfileIdByAppIndex(int32_t osAccountId, int32_t appIndex, int32_t &subProfileId) const override;
int32_t GetAppIndexBySubProfileId(int32_t osAccountId, int32_t subProfileId, int32_t &appIndex) const override;

// account_delegate_normal_impl.cpp 实现（直接调用 OsAccountSubProfileClient）:
int32_t AccountDelegateNormalImpl::GetSubProfileIdByToken(uint32_t tokenId, int32_t &subProfileId) const
{
    auto ret = AccountSA::OsAccountSubProfileClient::GetInstance().GetOsAccountSubProfileId(tokenId, subProfileId);
    if (ret != 0) {
        ZLOGE("GetOsAccountSubProfileId by token failed, ret:%{public}d, tokenId:0x%{public}x", ret, tokenId);
        subProfileId = -1;
    }
    return ret;
}

int32_t AccountDelegateNormalImpl::GetForegroundSubProfileId(int32_t osAccountId, int32_t &subProfileId) const
{
    auto ret = AccountSA::OsAccountSubProfileClient::GetInstance().GetOsAccountForegroundSubProfileId(
        osAccountId, subProfileId);
    if (ret != 0) {
        ZLOGE("GetOsAccountForegroundSubProfileId failed, ret:%{public}d, osAccountId:%{public}d", ret, osAccountId);
        subProfileId = -1;
    }
    return ret;
}

int32_t AccountDelegateNormalImpl::GetSubProfileIdByAppIndex(
    int32_t osAccountId, int32_t appIndex, int32_t &subProfileId) const
{
    auto ret = AccountSA::OsAccountSubProfileClient::GetInstance().GetOsAccountSubProfileId(
        osAccountId, appIndex, subProfileId);
    if (ret != 0) {
        ZLOGE("GetOsAccountSubProfileId by appIndex failed, ret:%{public}d, osAccountId:%{public}d, appIndex:%{public}d",
            ret, osAccountId, appIndex);
        subProfileId = -1;
    }
    return ret;
}

int32_t AccountDelegateNormalImpl::GetAppIndexBySubProfileId(
    int32_t osAccountId, int32_t subProfileId, int32_t &appIndex) const
{
    AccountSA::OsAccountSubspaceResult subspaceResult;
    AccountSA::OhosAccountInfo distributedInfo;
    auto ret = AccountSA::OsAccountSubProfileClient::GetInstance().GetOsAccountSubProfile(
        osAccountId, subProfileId, subspaceResult, distributedInfo);
    if (ret != 0) {
        ZLOGE("GetOsAccountSubProfile failed, ret:%{public}d, osAccountId:%{public}d, subProfileId:%{public}d",
            ret, osAccountId, subProfileId);
        appIndex = -1;
        return ret;
    }
    appIndex = subspaceResult.index;
    return ret;
}
```

**编译依赖**：`account_delegate_normal_impl.cpp` 需新增 `#include "os_account_subprofile_client.h"`，并依赖 `os_account:os_account_innerkits`（已存在）。注意真实代码中 `GetSubProfileIdByToken` 不含 `userId` 入参（账号子系统接口 `GetOsAccountSubProfileId(uint32_t tokenId, ...)` 无需 userId）。

#### 5.1.3 Stub 实现 — `adapter/account/src/account_delegate_default_impl.h/.cpp`

```cpp
// account_delegate_default_impl.h 新增:
int32_t GetSubProfileIdByToken(uint32_t tokenId, int32_t &subProfileId) const override;
int32_t GetForegroundSubProfileId(int32_t osAccountId, int32_t &subProfileId) const override;
int32_t GetSubProfileIdByAppIndex(int32_t osAccountId, int32_t appIndex, int32_t &subProfileId) const override;
int32_t GetAppIndexBySubProfileId(int32_t osAccountId, int32_t subProfileId, int32_t &appIndex) const override;

// account_delegate_default_impl.cpp 实现:
int32_t AccountDelegateDefaultImpl::GetSubProfileIdByToken(uint32_t tokenId, int32_t &subProfileId) const
{
    ZLOGD("no account part.");
    subProfileId = -1;
    return -1;
}

int32_t AccountDelegateDefaultImpl::GetForegroundSubProfileId(int32_t osAccountId, int32_t &subProfileId) const
{
    ZLOGD("no account part.");
    subProfileId = -1;
    return -1;
}

int32_t AccountDelegateDefaultImpl::GetSubProfileIdByAppIndex(
    int32_t osAccountId, int32_t appIndex, int32_t &subProfileId) const
{
    ZLOGD("no account part.");
    subProfileId = -1;
    return -1;
}

int32_t AccountDelegateDefaultImpl::GetAppIndexBySubProfileId(
    int32_t osAccountId, int32_t subProfileId, int32_t &appIndex) const
{
    ZLOGD("no account part.");
    appIndex = -1;
    return -1;
}
```

---

### 5.2 URI 解析层

#### `service/data_share/common/utils.h`

已新增：
- 常量：`static constexpr const char *ACCOUNT_ID = "accountId";`
- 方法：`static bool GetAccountIdFromProxyURI(const std::string &uri, std::string &accountId);`

#### `service/data_share/common/utils.cpp`

已实现（返回 string 格式，与 `GetAppIndexFromProxyURI` 风格一致）。

**使用时**：在业务逻辑层通过 `URIUtils::Strtoul` 转为 `int32_t`。

---

### 5.3 ProfileInfo 配置层 — accountIsolation

#### `service/data_share/data_share_profile_config.h`

`ProfileInfo` 结构体新增顶层字段：

```cpp
struct ProfileInfo : public DistributedData::Serializable {
    // ... 现有字段 ...
    bool accountIsolation = false;   // 提供方是否按账号隔离数据
    // ...
};
```

#### `service/data_share/data_share_profile_config.cpp`

`ProfileInfo::Marshal` 新增：
```cpp
SetValue(node[GET_NAME(accountIsolation)], accountIsolation);
```

`ProfileInfo::Unmarshal` 新增：
```cpp
GetValue(node, GET_NAME(accountIsolation), accountIsolation);
```

#### module.json5 配置示例

提供方在 `dataProperties` profile JSON 中声明：

```json
{
  "isSilentProxyEnable": true,
  "path": "store/table",
  "accountIsolation": true
}
```

或按 URI 粒度配置（仅在 ProfileInfo 顶层生效）：

```json
{
  "isSilentProxyEnable": true,
  "path": "store/table",
  "accountIsolation": true,
  "tableConfig": [{
    "uri": "datashare:///bundle/module/store/table",
    "crossUserMode": 2
  }]
}
```

---

### 5.4 数据模型层

#### `service/data_share/data_provider_config.h` — ProviderInfo 新增字段

```cpp
struct ProviderInfo {
    // ... 现有字段 ...
    int32_t accountId = -1;          // 有效 accountId (subProfileId), -1 表示未获取到
    bool accountIsolation = false;   // 提供方是否按账号隔离数据
    std::string dataDir;             // 无分身+accountIsolation 场景下拼接的数据库路径（含 accountId）
    bool queryByPath = false;        // 是否按 dataDir 路径查询元数据
    // ...
};
```

**字段语义**：
- `accountId = -1`：未获取到有效 accountId（可能是非账号隔离场景或获取失败）
- `accountId > 0`：有效的 subProfileId
- `accountIsolation = false`：提供方不按账号隔离，使用原有 DB 查找逻辑
- `accountIsolation = true`：提供方按账号隔离；有分身走 appIndex 维度，无分身走 dataDir 维度
- `dataDir`：仅无分身+accountIsolation=true 时填充，accountId 与 storeName 拼接的数据库路径
- `queryByPath`：为 true 时 QueryMetaData 按 `GetStoreMetaKeyWithPath(dataDir)` 查询

#### `service/data_share/common/context.h` — Context 新增字段

```cpp
class Context {
public:
    // ... 现有字段 ...
    int32_t accountId = -1;   // 账号隔离场景下的有效 subProfileId
    // ...
};
```

---

### 5.5 访问方身份判断 — ResolveAccessorAccountId

#### `service/data_share/data_provider_config.cpp` — `ResolveAccessorAccountId()`

构造函数在 `providerInfo_` 初始化和 `uriConfig_` 解析之后调用 `ResolveAccessorAccountId()`（`#ifdef ACCOUNT_ISOLATION_ENABLED` 包裹）。

**编译隔离**：此方法仅在 `ACCOUNT_ISOLATION_ENABLED` 编译开关下存在。
**运行隔离**：方法内首个判断为 `IsCarDevice()`，非车机直接返回。

**判空校验**：取 `AccountDelegate` 实例后必须判空，为空则记录日志并跳过，避免空指针。

**逻辑优化**：先判断访问方是否系统应用，再处理三方应用。有分身与否通过 `BundleMgrProxy::GetCloneAppIndexes` 判定（与现有代码一致）。

```cpp
void DataProviderConfig::ResolveAccessorAccountId()
{
    if (!IsCarDevice()) {
        return;   // 运行隔离：非车机直接跳过
    }
    auto *delegate = AccountDelegate::GetInstance();
    if (delegate == nullptr) {       // 判空校验
        ZLOGE("AccountDelegate is null, account isolation skipped");
        return;
    }
    auto callerTokenId = IPCSkeleton::GetCallingTokenID();
    auto callerTokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(callerTokenId);
    if (callerTokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE ||
        callerTokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL) {
        // ① 系统应用：解析 URI 中的 accountId
        std::string accountIdStr;
        URIUtils::GetAccountIdFromProxyURI(providerInfo_.uri, accountIdStr);
        if (!accountIdStr.empty()) {
            providerInfo_.accountId = atoi(accountIdStr.c_str());
        }
        if (providerInfo_.accountId <= 0) {
            // 系统应用未指定 accountId，取当前前台账号
            delegate->GetForegroundSubProfileId(providerInfo_.visitedUserId, providerInfo_.accountId);
        }
        return;
    }
    if (callerTokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_HAP) {
        // ② 三方应用：判断是否有分身
        Security::AccessToken::HapTokenInfo hapInfo;
        auto ret = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(callerTokenId, hapInfo);
        if (ret != Security::AccessToken::RET_SUCCESS) {
            return;
        }
        std::vector<int32_t> cloneAppIndexes;
        auto errCode = BundleMgrProxy::GetInstance()->GetCloneAppIndexes(
            hapInfo.bundleName, cloneAppIndexes, providerInfo_.visitedUserId);
        if (errCode == 0 && !cloneAppIndexes.empty()) {
            // 有分身三方应用：通过 tokenId 获取 subProfileId（忽略 URI accountId）
            delegate->GetSubProfileIdByToken(callerTokenId, providerInfo_.accountId);
        } else {
            // 无分身应用：取当前前台账号
            delegate->GetForegroundSubProfileId(providerInfo_.visitedUserId, providerInfo_.accountId);
        }
    }
}
```

> **接口签名变更**：`GetSubProfileIdByToken` 去掉了原有的 `userId` 入参（账号子系统 `GetOsAccountSubProfileId(uint32_t tokenId, ...)` 无需 userId）。

#### 访问方身份判断汇总表

| 访问方类型 | token 类型 | accountId 来源 | URI accountId |
|-----------|-----------|---------------|--------------|
| 系统服务/Shell | TOKEN_NATIVE / TOKEN_SHELL | URI `?accountId=X`；未指定则 `GetForegroundSubProfileId(visitedUserId)` | **使用（可选）** |
| 有分身三方应用 | TOKEN_HAP | `GetSubProfileIdByToken(callerTokenId)` | **忽略** |
| 无分身三方应用 | TOKEN_HAP | `GetForegroundSubProfileId(visitedUserId)` | **忽略** |

---

### 5.6 提供方身份识别 — accountIsolation 传递 + 身份解析

#### 5.6.1 accountIsolation 配置传递 — `data_provider_config.cpp`

在 `GetFromDataProperties` 中传递 `accountIsolation`：

```cpp
int DataProviderConfig::GetFromDataProperties(const ProfileInfo &profileInfo,
    const std::string &moduleName)
{
    // ... 现有逻辑 ...
    providerInfo_.accountIsolation = profileInfo.accountIsolation;   // 新增
    // ...
}
```

#### 5.6.2 提供方身份识别 — `data_share_service_impl.cpp` ResolveProviderAppIndex

`ExecuteEx` 中 `GetProviderInfo()` 完成后、构建 `DbConfig` 之前调用 `ResolveProviderAppIndex(providerInfo)`（`#ifdef ACCOUNT_ISOLATION_ENABLED` 包裹）。

**编译隔离**：此方法仅在 `ACCOUNT_ISOLATION_ENABLED` 编译开关下存在。
**运行隔离**：方法内首个判断为 `IsCarDevice()`，非车机直接返回。

按**提供方是否有分身**分两路处理（有无分身通过 `BundleMgrProxy::GetCloneAppIndexes` 判定，与访问方一致）：

```cpp
void DataShareServiceImpl::ResolveProviderAppIndex(ProviderInfo &providerInfo)
{
    if (!IsCarDevice() || providerInfo.accountId <= 0) {
        return;   // 运行隔离 / 无有效 accountId：保持原有逻辑
    }
    auto *delegate = AccountDelegate::GetInstance();
    if (delegate == nullptr) {
        return;
    }
    // 判断提供方是否有分身
    std::vector<int32_t> cloneAppIndexes;
    BundleMgrProxy::GetInstance()->GetCloneAppIndexes(
        providerInfo.bundleName, cloneAppIndexes, providerInfo.visitedUserId);
    if (!cloneAppIndexes.empty()) {
        // ① 有分身应用：accountId → appIndex，用 appIndex(instanceId) 查 meta
        int32_t providerAppIndex = 0;
        delegate->GetAppIndexBySubProfileId(
            providerInfo.visitedUserId, providerInfo.accountId, providerAppIndex);
        if (providerAppIndex > 0) {
            providerInfo.appIndex = providerAppIndex;
        }
        // 后续 QueryMetaData 用 StoreMetaMapping::GetKey()（含 instanceId）查 meta
    } else if (providerInfo.accountIsolation) {
        // ② 无分身应用 + 区分账号：accountId 与 storeName 拼接成 dataDir，按路径查 meta
        providerInfo.dataDir = ConstructAccountDataDir(providerInfo.accountId, providerInfo.storeName);
        providerInfo.queryByPath = true;
        // 后续 QueryMetaData 用 GetStoreMetaKeyWithPath(dataDir) 查 meta
    }
    // ③ 无分身应用 + 不区分账号(accountIsolation=false)：不处理，直接用 storeName 查 meta（原有逻辑）
}
```

#### 5.6.3 无分身 accountIsolation 数据库路径拼接与元数据查询

**路径拼接规则**：提供方为每个账号注册元数据时，数据库路径（dataDir）按约定格式包含 accountId（如 `{base_path}/{accountId}/{storeName}`）。查询方按相同规则拼接 accountId 与 storeName 构造 dataDir（`ConstructAccountDataDir`），保证与注册侧一致。

**元数据查询方式**：无分身应用无 instanceId 可用，改用含 dataDir 的元数据键查询：

| 场景 | 元数据查询键 | 查询方法 |
|------|------------|---------|
| 有分身应用 | `StoreMetaMapping::GetKey()`（含 instanceId=appIndex，不含 dataDir） | `QueryMetaData(bundle, store, user, appIndex)` |
| 无分身 + accountIsolation=true | `StoreMetaMapping::GetStoreMetaKeyWithPath(dataDir)`（含 dataDir） | `QueryMetaData` 扩展按 dataDir 查询 |
| 无分身 + accountIsolation=false | `StoreMetaMapping::GetKey()`（instanceId=0，不含 dataDir） | `QueryMetaData(bundle, store, user, 0)`（原有逻辑） |

> **改造点**：
> - `QueryMetaData`（`data_share_db_config.cpp:36-52`）当前仅按 `StoreMetaMapping::GetKey()` 查询，需扩展支持按 `GetStoreMetaKeyWithPath(dataDir)` 查询（无分身 accountIsolation 场景）。
> - `ProviderInfo` 需新增 `dataDir`（string）与 `queryByPath`（bool）字段，经 `DbConfig` 透传给 `QueryMetaData`。

#### 提供方身份判断汇总表

| 提供方类型 | 有无分身 | accountIsolation | 元数据查找方式 |
|-----------|---------|-----------------|--------------|
| 有分身应用 | 是 | true | accountId → `GetAppIndexBySubProfileId` → appIndex → `GetKey()`(含instanceId) → meta → dataDir |
| 无分身应用 | 否 | true | accountId + storeName 拼接 dataDir → `GetStoreMetaKeyWithPath(dataDir)` → meta → dataDir |
| 无分身应用 | 否 | false | storeName → `GetKey()`(instanceId=0) → meta → dataDir（原有逻辑） |

---

### 5.7 GetSilentProxyStatus — calledTokenId 构造修改

#### `data_share_service_impl.cpp` — GetSilentProxyStatus

当前逻辑使用 URI 中的 appIndex 构造 `calledTokenId`。账号隔离场景下，**仅提供方有分身时**才需 accountId → appIndex 转换（不同账号对应不同分身，calledTokenId 不同）；无分身应用 appIndex=0，无需转换。

**问题**：此函数当前不获取提供方的 `accountIsolation` 配置，无法判断是否需要 accountId 转换。

**方案**：在此处也通过 `DataShareProfileConfig` 读取提供方的 `accountIsolation` 配置：

**编译隔离**：此段逻辑仅在 `ACCOUNT_ISOLATION_ENABLED` 编译开关下存在。
**运行隔离**：此段逻辑仅在 `IsCarDevice()` 返回 true 时执行。

```cpp
int32_t DataShareServiceImpl::GetSilentProxyStatus(const std::string &uri, bool isCreateHelper)
{
    // ... 现有逻辑到 calledBundleName, appIndex ...

#ifdef ACCOUNT_ISOLATION_ENABLED
    // ===== 新增: 账号隔离场景下 accountId → appIndex（仅车机设备） =====
    if (IsCarDevice()) {
        std::string accountIdStr;
        URIUtils::GetAccountIdFromProxyURI(uri, accountIdStr);
        if (!accountIdStr.empty()) {
            // 检查提供方是否按账号隔离
            std::map<std::string, ProfileInfo> profileInfos;
            if (DataShareProfileConfig::GetProfileInfo(calledBundleName, currentUserId, profileInfos)) {
                for (auto &[key, profileInfo] : profileInfos) {
                    if (profileInfo.accountIsolation) {
                        // 仅提供方有分身时才转换 appIndex；无分身 appIndex 保持 0
                        std::vector<int32_t> cloneAppIndexes;
                        BundleMgrProxy::GetInstance()->GetCloneAppIndexes(
                            calledBundleName, cloneAppIndexes, currentUserId);
                        if (!cloneAppIndexes.empty()) {
                            auto [success, data] = URIUtils::Strtoul(accountIdStr);
                            if (success) {
                                int32_t providerAppIndex = 0;
                                AccountDelegate::GetInstance()->GetAppIndexBySubProfileId(
                                    currentUserId, static_cast<int32_t>(data), providerAppIndex);
                                if (providerAppIndex > 0) {
                                    appIndex = providerAppIndex;   // 覆盖 URI appIndex
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
#endif

    uint32_t calledTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(
        currentUserId, calledBundleName, appIndex);
    // ... 现有后续逻辑 ...
}
```

---

### 5.8 LoadConfigCommonStrategy — 策略链 accountId 解析

#### `service/data_share/strategies/general/load_config_common_strategy.cpp`

`operator()(context)` 中通过 `#ifdef ACCOUNT_ISOLATION_ENABLED` 调用 `ResolveContextAccountId(context)`。该方法的解析逻辑与 `DataProviderConfig::ResolveAccessorAccountId` 保持一致。

**编译隔离**：`ResolveContextAccountId` 仅在 `ACCOUNT_ISOLATION_ENABLED` 编译开关下存在。
**运行隔离**：方法内首个判断为 `IsCarDevice()`，非车机直接返回。

```cpp
void LoadConfigCommonStrategy::ResolveContextAccountId(std::shared_ptr<Context> context)
{
    if (!IsCarDevice()) {
        return;   // 运行隔离：非车机直接跳过
    }
    auto *delegate = AccountDelegate::GetInstance();
    if (delegate == nullptr) {       // 判空校验
        ZLOGE("AccountDelegate is null, account isolation skipped");
        return;
    }
    auto callerTokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(context->callerTokenId);
    if (callerTokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE ||
        callerTokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL) {
        // ① 系统应用：解析 URI 中的 accountId
        std::string accountIdStr;
        URIUtils::GetAccountIdFromProxyURI(context->uri, accountIdStr);
        if (!accountIdStr.empty()) {
            context->accountId = atoi(accountIdStr.c_str());
        }
        if (context->accountId <= 0) {
            // 系统应用未指定 accountId，取当前前台账号
            delegate->GetForegroundSubProfileId(context->visitedUserId, context->accountId);
        }
        return;
    }
    if (callerTokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_HAP) {
        // ② 三方应用：判断是否有分身
        Security::AccessToken::HapTokenInfo hapInfo;
        auto ret = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(context->callerTokenId, hapInfo);
        if (ret != Security::AccessToken::RET_SUCCESS) {
            return;
        }
        std::vector<int32_t> cloneAppIndexes;
        auto errCode = BundleMgrProxy::GetInstance()->GetCloneAppIndexes(
            hapInfo.bundleName, cloneAppIndexes, context->visitedUserId);
        if (errCode == 0 && !cloneAppIndexes.empty()) {
            // 有分身三方应用：通过 tokenId 获取 subProfileId
            delegate->GetSubProfileIdByToken(context->callerTokenId, context->accountId);
        } else {
            // 无分身应用：取当前前台账号
            delegate->GetForegroundSubProfileId(context->visitedUserId, context->accountId);
        }
    }
}
```

---

### 5.9 跨账号权限验证

现有 `VerifyAcrossAccountsPermission` 基于 userId 跨用户权限检查。账号隔离场景下的补充：

- **同 userId 不同 accountId** 之间的访问，视为跨账号访问
- 复用现有 `acrossAccountsPermission` 字段（默认 `ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS`）
- 在 `ExecuteEx` 中，`VerifyAcrossAccountsPermission` 之后的权限检查链已覆盖此场景（系统应用可通过指定 `?accountId=xxx` 跨账号访问）

**注意**：当前 `VerifyAcrossAccountsPermission` 仅在 `currentUserId != visitedUserId` 时验证。在账号隔离场景下（`currentUserId == visitedUserId` 但 `accessorAccountId != providerAccountId`），此函数会直接通过（因为同一 userId）。这是符合预期的行为——系统应用有权跨账号访问数据（由 URI accountId 参数驱动），三方应用只能访问自己账号或前台账号的数据（由身份判断逻辑保证）。

**无需额外修改**：访问方身份判断逻辑已保证：
- 有分身三方应用只能访问自己 accountId 对应的数据
- 无分身三方应用只能访问前台 accountId 对应的数据
- 系统应用可指定任意 accountId（权限由 allowList + readPermission/writePermission 控制）

---

## 6. 变更文件清单

| # | 文件路径 | 变更类型 | 变更内容 |
|---|---------|---------|---------|
| 1 | `datamgr_service.gni` | 新增配置 | `datamgr_service_account_isolation` 编译开关（默认 false） |
| 2 | `service/data_share/BUILD.gn` | 修改配置 | 条件定义 `ACCOUNT_ISOLATION_ENABLED` |
| 3 | `service/data_share/common/common_utils.h` | 新增方法 | `IsCarDevice()` 声明 |
| 4 | `service/data_share/common/common_utils.cpp` | 新增方法 | `IsCarDevice()` 实现（基于 DeviceManagerAdapter） |
| 5 | `framework/include/account/account_delegate.h` | 新增方法 | 4 个纯虚方法：GetSubProfileIdByToken(去userId), GetForegroundSubProfileId, GetSubProfileIdByAppIndex, GetAppIndexBySubProfileId |
| 6 | `adapter/account/src/account_delegate_normal_impl.h` | 新增方法 | 4 个方法声明 |
| 7 | `adapter/account/src/account_delegate_normal_impl.cpp` | 新增方法 | 4 个方法实现（直接调用 OsAccountSubProfileClient） |
| 8 | `adapter/account/src/account_delegate_default_impl.h` | 新增方法 | 4 个方法声明 |
| 9 | `adapter/account/src/account_delegate_default_impl.cpp` | 新增方法 | 4 个 Stub 方法实现 |
| 10 | `service/data_share/common/utils.h` | 已完成 | ACCOUNT_ID 常量 + GetAccountIdFromProxyURI 声明 |
| 11 | `service/data_share/common/utils.cpp` | 已完成 | GetAccountIdFromProxyURI 实现 |
| 12 | `service/data_share/data_share_profile_config.h` | 新增字段 | ProfileInfo.accountIsolation (bool) |
| 13 | `service/data_share/data_share_profile_config.cpp` | 新增序列化 | ProfileInfo Marshal/Unmarshal 新增 accountIsolation |
| 14 | `service/data_share/data_provider_config.h` | 新增字段 | ProviderInfo.accountId (int32_t), ProviderInfo.accountIsolation (bool), ProviderInfo.dataDir (string), ProviderInfo.queryByPath (bool) |
| 15 | `service/data_share/data_provider_config.cpp` | 修改逻辑 | 构造函数新增访问方身份判断（`#ifdef ACCOUNT_ISOLATION_ENABLED` + `IsCarDevice()` 门控）；GetFromDataProperties 传递 accountIsolation |
| 16 | `service/data_share/common/context.h` | 新增字段 | Context.accountId (int32_t) |
| 17 | `service/data_share/data_share_service_impl.cpp` | 修改逻辑 | ExecuteEx 新增 ResolveProviderAppIndex 提供方身份识别（有分身→appIndex/无分身→dataDir，`#ifdef` + `IsCarDevice()` 门控）；GetSilentProxyStatus 有分身才转 appIndex |
| 18 | `service/data_share/strategies/general/load_config_common_strategy.cpp` | 修改逻辑 | operator() 新增 accountId 解析（`#ifdef` + `IsCarDevice()` 门控） |
| 19 | `service/data_share/data_share_db_config.cpp` | 修改逻辑 | QueryMetaData 扩展按 dataDir 路径查询（GetStoreMetaKeyWithPath，无分身 accountIsolation 场景） |

---

## 7. 风险评估

### 7.1 外部依赖风险

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| 账号子系统 API 行为与预期不一致 | 中 | 接口参数与 `os_account_subprofile_client.h` 对齐；上线后集成测试 |
| `ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE` 宏未启用 | 中 | 车机产品编译需确保该宏已定义，否则 `OsAccountSubProfileClient` 调用编译失败 |
| AccountDelegate 实例为空（无 account 部件） | 中 | 访问方身份判断入口判空校验，为空则跳过降级为非账号隔离访问 |

### 7.2 设备类型隔离风险

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| 非车机设备意外启用账号隔离 | **高** | 双层隔离（编译开关 + 运行时 IsCarDevice() 检查）；`datamgr_service_account_isolation` 默认 false |
| 编译开关在车机产品配置中遗漏 | 中 | 车机产品 gn 配置必须显式设置 `datamgr_service_account_isolation = true`；文档明确标注 |
| IsCarDevice() 误判（DeviceManagerAdapter 初始化延迟） | 中 | GetLocalDevice() 在服务启动后已初始化；若 uuid 为空则返回 UNKNOWN（非 CAR），自动降级 |
| 通用二进制在不同设备运行 | 低 | 运行时 IsCarDevice() 检查覆盖此场景；非车机自动跳过 |
| `#ifdef` 代码块与非 `#ifdef` 代码块耦合 | 中 | 门控逻辑仅包裹新增部分，不修改现有代码；原有逻辑在 `#else`/无 `#ifdef` 时完整保留 |

### 7.3 数据兼容性风险

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| StoreMetaData 元数据存储键格式 | **无** | 复用现有 instanceId + dataDir 机制，不修改键格式；账号隔离仅改变 appIndex 来源（accountId 转换） |
| ProfileInfo 新增字段向后兼容 | 低 | accountIsolation 默认 false，旧 profile JSON 缺少此字段时 GetValue 返回默认值 |
| accountId=-1 与无 accountId 的区分 | 中 | 明确语义：-1=获取失败/不支持，0=未参与账号流程；代码中统一判断 `accountId > 0` |
| 非车机设备上 accountIsolation 配置存在但无效 | 低 | 编译开关关闭时 accountIsolation 字段虽可解析但逻辑不生效；运行时 IsCarDevice() 也做二次拦截 |

### 7.4 功能风险

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| 有分身应用 accountId→appIndex 转换失败 | 中 | 回退到原 appIndex，降级为非账号隔离访问；日志告警 |
| 无分身应用 dataDir 路径拼接不一致 | **高** | 拼接规则需与提供方注册侧严格一致；不一致则查不到 meta，降级失败需日志告警 |
| appIndex 覆盖冲突（URI 指定 vs accountId 转换） | 低 | 仅提供方有分身且 accountIsolation=true 时覆盖，其他场景保持原值 |
| QueryMetaData 按 dataDir 查询未命中 | 中 | 无分身 accountIsolation 场景 meta 未注册时返回 E_DB_NOT_EXIST；提示提供方注册元数据 |
| GetSilentProxyStatus 无法读取 accountIsolation | 中 | 通过 DataShareProfileConfig 读取；性能影响需评估（额外 BMS 查询） |
| 无分身应用获取前台 accountId 失败 | 中 | accountId=-1 降级为非账号隔离访问 |

### 7.5 性能风险

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| 每次请求增加 AccountDelegate 调用 | 低 | 账号子系统 API 应为轻量查询；可后续引入缓存 |
| GetSilentProxyStatus 增加 ProfileConfig 读取 | 中 | 可考虑缓存 accountIsolation 状态 |

---

## 8. 验证计划

### 8.1 构建

```bash
./build.sh --product-name <product> --build-target datamgr_service
```

### 8.2 Lint

```bash
git-clang-format
```

### 8.3 单元测试覆盖

| 测试目标 | 测试场景 | 优先级 |
|---------|---------|--------|
| `IsCarDevice()` | DEVICE_TYPE_CAR / DEVICE_TYPE_PHONE / DEVICE_TYPE_PAD / DEVICE_TYPE_UNKNOWN / uuid 为空 | P0 |
| `URIUtils::GetAccountIdFromProxyURI` | 正常参数、空参数、非法参数、多参数组合 | P0 |
| `AccountDelegate::GetSubProfileIdByToken` | TOKEN_HAP / 0 / 无效 tokenId（无 userId 入参） | P0 |
| `AccountDelegate::GetForegroundSubProfileId` | 正常参数 / osAccountId=0 / 无前台账号 | P0 |
| `AccountDelegate::GetSubProfileIdByAppIndex` | 正常参数 / osAccountId=0 / appIndex=0 | P0 |
| `AccountDelegate::GetAppIndexBySubProfileId` | 正常参数 / subProfileId=-1 / osAccountId=0 | P0 |
| `ProfileInfo::Unmarshal` accountIsolation | 有字段 / 无字段（向后兼容） / 非布尔值 | P0 |
| 访问方身份判断 | 系统应用(指定accountId/未指定取前台) / 有分身三方 / 无分身应用 / delegate为空 | P0 |
| 提供方身份识别 | 有分身(accountId→appIndex) / 无分身+accountIsolation(拼dataDir) / 无分身+非隔离 / accountId=-1 | P0 |
| **设备类型门控** | **IsCarDevice()=true 时执行隔离逻辑 / IsCarDevice()=false 时跳过隔离逻辑** | **P0** |
| **编译开关门控** | **ACCOUNT_ISOLATION_ENABLED 未定义时，相关代码不编译（构建验证）** | **P0** |
| **非车机设备 URI accountId 忽略** | **非车机设备传入 ?accountId=xxx，accountId 保持 -1** | **P0** |
| GetSilentProxyStatus accountId→appIndex | 有accountId+accountIsolation=true / 无accountId | P1 |
| LoadConfigCommonStrategy accountId 解析 | 各身份类型与 DataProviderConfig 一致 | P1 |

### 8.4 构建隔离验证

| 验证项 | 验证方法 | 预期结果 |
|--------|---------|---------|
| 非车机产品编译 | `datamgr_service_account_isolation=false` 构建 | 二进制中不含账号隔离相关代码（无 `ACCOUNT_ISOLATION_ENABLED` 符号） |
| 车机产品编译 | `datamgr_service_account_isolation=true` 构建 | 二进制中包含账号隔离代码（有 `ACCOUNT_ISOLATION_ENABLED` 符号） |
| 车机二进制非车机运行 | 车机二进制在手机模拟器运行 | `IsCarDevice()=false`，账号隔离逻辑自动跳过，原有流程正常 |

### 8.5 测试约束

- 测试用例必须包含显式断言（EXPECT_XXX / ASSERT_XXX）
- Mock AccountDelegate 时使用统一 Mock 类
- 所有测试用例文档头包含 `@tc.author: agent`
- UT 测试名称格式：被测方法_测试场景_预期结果
- 禁止不可能失败的断言（如 `EXPECT_TRUE(true)`）

---

## 9. 上线计划

### Phase 1: 框架搭建 + 设备门控（当前）

- **编译开关**：`datamgr_service.gni` 新增 `datamgr_service_account_isolation`（默认 false）
- **编译隔离**：`BUILD.gn` 条件定义 `ACCOUNT_ISOLATION_ENABLED`
- **运行隔离**：`common_utils.h/.cpp` 新增 `IsCarDevice()`
- AccountDelegate 接口定义 + 实现（NormalImpl 直连 OsAccountSubProfileClient，DefaultImpl Stub）
- URI 解析（已完成）
- ProfileInfo accountIsolation 配置
- ProviderInfo / Context 数据模型
- 访问方身份判断逻辑（`#ifdef ACCOUNT_ISOLATION_ENABLED` + `IsCarDevice()` 门控）
- 提供方身份识别逻辑 ResolveProviderAppIndex（`#ifdef ACCOUNT_ISOLATION_ENABLED` + `IsCarDevice()` 门控）
- **验证**：非车机编译构建成功 + 二进制不含账号隔离代码
- **行为**：API 已上线，账号隔离逻辑生效；delegate 为空或解析失败时 accountId=-1，降级为非账号隔离访问

### Phase 2: 车机产品配置 + 真实接入

- 车机产品 gn 配置显式设置 `datamgr_service_account_isolation = true`
- 确保账号子系统 `ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE` 宏已启用
- AccountDelegateNormalImpl 已直接调用 `OsAccountSubProfileClient`（API 已上线）
- 集成测试验证三个场景（仅在车机设备）
- 性能测试评估 AccountDelegate 调用开销
- **验证**：车机编译构建成功 + 非车机设备 IsCarDevice() 门控生效

### Phase 3: 配置与数据注册

- 提供方应用在 module.json5 中声明 accountIsolation=true
- 提供方应用为每个账号注册元数据（通过 DataShareExtension）
- 端到端验证：不同 accountId 访问不同数据库（仅在车机设备）

---

## 10. 附录

### 10.1 现有代码关键路径索引

| 概念 | 文件 | 行号 |
|------|------|------|
| 编译开关定义 | `datamgr_service.gni` | :43-91 |
| PLATFORM_PC 编译隔离 | `service/data_share/BUILD.gn` | :34-39 |
| ProviderInfo 定义 | `data_provider_config.h` | :34-59 |
| DataProviderConfig 构造函数 | `data_provider_config.cpp` | :36-62 |
| GetFromDataProperties | `data_provider_config.cpp` | :125-141 |
| GetFromExtensionProperties | `data_provider_config.cpp` | :143-159 |
| ExecuteEx | `data_share_service_impl.cpp` | :1274-1321 |
| GetSilentProxyStatus | `data_share_service_impl.cpp` | :1051-1089 |
| VerifyAcrossAccountsPermission | `data_share_service_impl.cpp` | :1260-1272 |
| LoadConfigCommonStrategy | `load_config_common_strategy.cpp` | :30-56 |
| StoreMetaData::GetKey（存储键，含 dataDir） | `store_meta_data.cpp` | :178-184 |
| StoreMetaMapping::GetKey（查询键，不含 dataDir） | `store_meta_data.cpp` | :325-331 |
| QueryMetaData | `data_share_db_config.cpp` | :36-52 |
| DbConfig 构建 | `data_share_service_impl.cpp` | :1310-1312 |
| GetCallerInfo | `data_share_service_impl.cpp` | :343-375 |
| AccountDelegate 接口 | `account_delegate.h` | :45-84 |
| AccountDelegateNormalImpl | `account_delegate_normal_impl.cpp` | 全文 |
| ProfileInfo 定义 | `data_share_profile_config.h` | :55-70 |
| Context 定义 | `context.h` | :34-69 |
| DEVICE_TYPE_CAR 定义 | `device_manager_adapter.h` | :44 |
| GetDeviceTypeByUuid | `device_manager_adapter.h` | :71 |
| GetLocalDevice | `device_manager_adapter.h` | :61 |
| IsSupportAutoSync（设备类型判断先例） | `rdb_service_impl.cpp` | :1591-1597 |
| CommonUtils / IsCarDevice 新增位置 | `common_utils.h/.cpp` | 全文 |

### 10.2 元数据键格式（StoreMetaData）

元数据**存储**键基于 `StoreMetaData::GetKey()`（`store_meta_data.cpp:178-184`），末尾包含 `dataDir` 维度：

```
instanceId == 0: {deviceId}:{user}:default:{bundleName}:{storeId}:{dataDir}
instanceId != 0: {deviceId}:{user}:default:{bundleName}:{storeId}:{instanceId}:{dataDir}
```

元数据**查询**键基于 `StoreMetaMapping::GetKey()`（`store_meta_data.cpp:325-331`），**不含 dataDir**，按前缀匹配存储条目：

```
instanceId == 0: {deviceId}:{user}:default:{bundleName}:{storeId}
instanceId != 0: {deviceId}:{user}:default:{bundleName}:{storeId}:{instanceId}
```

> 注：`QueryMetaData`（`data_share_db_config.cpp:36-52`）构造 `StoreMetaMapping` 并以 `GetKey()`（不含 dataDir）调用 `MetaDataManager::LoadMeta(key, storeMetaMapping, true)`，按前缀匹配到带 `dataDir` 的存储条目。

账号隔离场景下元数据查找分两路：有分身应用 accountId → appIndex 转换后，`instanceId = providerAppIndex`，用查询键（含 instanceId）查找；无分身应用 accountId + storeName 拼接 dataDir，用 `GetStoreMetaKeyWithPath(dataDir)` 查找。不同账号对应不同 `instanceId`（appIndex）或不同 `dataDir`（数据库路径）。

### 10.3 完整调用流程图

```
┌─────────────────────────────────────────────────────────────────┐
│                     客户端 IPC 调用                               │
│  Query/Insert/Update/Delete (uri 含 ?accountId=xxx)              │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  ★ 设备类型门控（双层隔离）                                       │
│  ├─ 编译层: ACCOUNT_ISOLATION_ENABLED 是否定义                    │
│  │   ├─ 未定义: 跳过全部账号隔离逻辑，走原有流程                   │
│  │   └─ 已定义: 进入运行时检查                                    │
│  ├─ 运行层: IsCarDevice()                                       │
│  │   ├─ false (非车机): 跳过账号隔离逻辑，走原有流程               │
│  │   └─ true (车机): 进入账号隔离流程                             │
└────────────────────────────┬────────────────────────────────────┘
                             │ (仅车机设备继续)
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  GetSilentProxyStatus                                           │
│  ├─ URIUtils::GetAppIndexFromProxyURI → appIndex               │
│  ├─ URIUtils::GetAccountIdFromProxyURI → accountIdStr          │
│  ├─ (有分身+accountIsolation) accountId→appIndex 覆盖          │
│  └─ AccessTokenKit::GetHapTokenID(userId, bundle, appIndex)    │
│     → calledTokenId                                             │
│  └─ IsSilentProxyEnable(calledTokenId, userId, bundle, uri)    │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  ExecuteEx                                                      │
│  ├─ DataProviderConfig(uri, tokenId)                            │
│  │   ├─ currentUserId = GetUserByToken(tokenId)                │
│  │   ├─ visitedUserId (from URI ?user= 或 currentUserId)       │
│  │   ├─ appIndex (from URI ?appIndex=)                         │
│  │   ├─ ★ accountId (访问方身份判断, delegate判空)              │
│  │   │   ├─ TOKEN_NATIVE/TOKEN_SHELL (系统应用):               │
│  │   │   │   GetAccountIdFromProxyURI(uri) → accountId         │
│  │   │   │   未指定则 GetForegroundSubProfileId(visitedUserId)  │
│  │   │   ├─ TOKEN_HAP + 有分身:                                 │
│  │   │   │   GetSubProfileIdByToken(tokenId) → accountId       │
│  │   │   └─ TOKEN_HAP + 无分身:                                 │
│  │   │       GetForegroundSubProfileId(visitedUserId)→accountId│
│  │   └─ uriConfig = GetUriConfig(uri)                          │
│  │                                                              │
│  ├─ GetProviderInfo()                                           │
│  │   ├─ GetFromDataShareConfig / GetFromProxyData /            │
│  │   │   GetFromExtension                                       │
│  │   ├─ ★ accountIsolation = profileInfo.accountIsolation      │
│  │   └─ 返回 ProviderInfo (含 accountId, accountIsolation)     │
│  │                                                              │
│  ├─ ★ 提供方身份识别 ResolveProviderAppIndex                    │
│  │   ├─ 有分身: GetAppIndexBySubProfileId → 覆盖 appIndex      │
│  │   ├─ 无分身+accountIsolation: accountId+storeName拼dataDir   │
│  │   └─ 无分身+非隔离: 不处理                                    │
│  │                                                              │
│  ├─ VerifyAcrossAccountsPermission                             │
│  ├─ CheckAllowList                                             │
│  ├─ VerifyPermission                                           │
│  │                                                              │
│  ├─ DataShareDbConfig::DbConfig                                │
│  │   {bundleName, storeName, userId, appIndex/dataDir,          │
│  │    queryByPath, hasExtension}                                │
│  │                                                              │
│  ├─ QueryMetaData                                              │
│  │   ├─ 有分身: GetKey()(含instanceId=appIndex)                │
│  │   ├─ 无分身+隔离: GetStoreMetaKeyWithPath(dataDir)          │
│  │   └─ 无分身+非隔离: GetKey()(instanceId=0)                  │
│  │   →StoreMetaData.dataDir→DB路径                             │
│  │                                                              │
│  └─ callback(providerInfo, metaData, dbDelegate)               │
│     → DB 操作 → 返回结果                                        │
└─────────────────────────────────────────────────────────────────┘

非车机设备流程（ACCOUNT_ISOLATION_ENABLED 未定义或 IsCarDevice()=false）：
  完整走原有 ExecuteEx 流程，accountId 保持 -1，accountIsolation 不生效，
  URI 中 ?accountId=xxx 参数被忽略，与现有行为完全一致。
```

### 10.4 术语对照表

| 术语 | 含义 | 来源 |
|------|------|------|
| accountId | 账号ID，即 subProfileId | 本方案 |
| subProfileId | 账号子系统中的账号标识 | 账号子系统 |
| osAccountId | OS用户ID，即现有 userId | 账号子系统 |
| appIndex / instIndex | 应用分身ID | AccessToken |
| subspaceResult.index | 账号对应的分身ID | 账号子系统 OsAccountSubspaceResult |
| accountIsolation | 提供方是否按账号隔离数据的配置项 | 本方案 |
| instanceId | StoreMetaMapping 中的分身维度，= appIndex | 元数据 |
| visitedUserId | 访问目标OS用户ID | 现有 |
| currentUserId | 调用方所属OS用户ID | 现有 |
| ACCOUNT_ISOLATION_ENABLED | 账号隔离编译开关宏 | 本方案 |
| datamgr_service_account_isolation | 账号隔离 GN 编译开关（默认 false） | 本方案 |
| IsCarDevice() | 运行时设备类型判断，仅车机返回 true | 本方案 |
| DEVICE_TYPE_CAR | 车机设备类型枚举值 0x83 | DeviceManagerAdapter |

### 10.5 设备类型隔离实现规范

所有账号隔离相关的新增代码必须遵循以下规范：

1. **编译隔离**：使用 `#ifdef ACCOUNT_ISOLATION_ENABLED` 包裹，不得使用其他宏名
2. **运行隔离**：在 `#ifdef` 块内，首个逻辑判断必须是 `if (IsCarDevice())`，不得跳过此检查
3. **降级安全**：`#ifdef` 块外的代码（原有逻辑）必须完整可运行，不依赖任何 `#ifdef` 块内的变量或函数
4. **变量默认值**：新增字段（如 `accountId`）的默认值必须保证原有逻辑正确（accountId=-1 表示无效）
5. **日志安全**：`#ifdef` 块内的日志在非车机编译时不存在，不影响日志标签注册

```cpp
// 规范模板：
#ifdef ACCOUNT_ISOLATION_ENABLED
    if (IsCarDevice()) {
        // 账号隔离逻辑
    }
#endif
    // 原有逻辑继续，不依赖上方任何变量赋值
```
