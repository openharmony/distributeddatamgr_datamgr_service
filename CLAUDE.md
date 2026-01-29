# CLAUDE.md

This file provides guidance for Claude Code (claude.ai/code) when working in this codebase.

## Project Overview

This is OpenHarmony's **Distributed Data Management Service** (distributeddatamgr_datamgr_service), the core implementation of OpenHarmony's distributed data management subsystem.

**Core Architecture Characteristic**: Adopts a **client-side storage, server-side management** separation architecture
- **Client (application process)**: Responsible for actual data storage and access, data is stored in the application's local database
- **Server (this service)**: Responsible for metadata management, data sync coordination, silent access, permission management, backup & recovery, and other management functions

**Five Core Capabilities**:
1. **KV/RDB Cross-Device Sync**: Automatic data sync and consistency guarantee between devices
2. **Distributed Object Persistence**: Complete lifecycle management of distributed objects
3. **Cross-App Data Silent Sharing**: Access data without launching provider process
4. **Device-Cloud Sync**: Sync database data to the cloud
5. **Unified Data Management Framework (UDMF)**: System-level unified standard for data management

**Data Isolation**: Uses (user, application, database) triplet for strict data isolation

## Architecture Overview

**Three-Layer Architecture**:
1. **App Layer** (`app/`): Service startup entry, Feature dispatch, event distribution
2. **Service Layer** (`service/`): Business logic implementation, sync coordination, permission management (includes adapters)
3. **Framework Layer** (`framework/`): Abstract interface definition, dependency injection container, common capability management

**Key Design Principles**:
- **Adapters are part of service layer**: Physical location in `adapter/` directory, linked to service layer at build time
- **Dependency Injection**: Framework layer defines interfaces → Adapters implement interfaces and call system services → Register to framework layer → Service layer uses
- **Feature Architecture**: Each service registers as a Feature to FeatureSystem, app layer dispatches through FeatureSystem

**Detailed Documentation**: For detailed introduction to each layer, see README.md in corresponding directories:
- App Layer: [services/distributeddataservice/app/README.md](services/distributeddataservice/app/README.md)
- Service Layer: [services/distributeddataservice/service/README.md](services/distributeddataservice/service/README.md)
- Framework Layer: [services/distributeddataservice/framework/README.md](services/distributeddataservice/framework/README.md)
- Adapter: [services/distributeddataservice/adapter/README.md](services/distributeddataservice/adapter/README.md)

## Key Concepts

### 1. GeneralStore (Server Database Handle)
- **Abstract handle for server-side database access**, not client abstraction
- Provides capabilities for Features (device-device sync, device-cloud sync, etc.) to access databases in the server process
- **Main Methods**: Insert/Replace/Update/Delete, Query, Sync, Execute, Watch/Unwatch
- Acquired and managed uniformly through AutoCache

### 2. AutoCache (Database Handle Management)
- **Server database handle management class**, provides caching and lifecycle management
- **Acquire Handle**: Get database handle via `GetStore()` or `GetDBStore()`
- **Handle Reclamation**: Automatic garbage collection mechanism (configurable interval), timely release of unused handles
- **Observer Management**: Support setting Watcher to listen for data changes
- **Important**: Do not retain handles acquired from AutoCache for long, release as soon as possible after use

**Usage Example**:
```cpp
auto &cache = AutoCache::GetInstance();
auto [status, store] = cache.GetDBStore(metaData, watchers);
store->Insert("table_name", std::move(value));
// Handle will be automatically reclaimed, do not retain for long
```

### 3. FeatureSystem (Feature Management System)
- **Feature registration and management system provided by framework layer**
- **Feature Registration**: Each Feature (RDB, KVDB, Cloud, Object, etc.) registers via `RegisterCreator()`
- **Feature Acquisition**: App layer acquires and instantiates Features via `GetCreator()`
- **Lifecycle Management**: Manages Feature initialization, binding, destruction
- **Event Distribution**: Distributes system events (app install/uninstall, user switch, device online/offline, etc.) to each Feature
- **Key Constraint**: App layer must not directly depend on Feature implementations, manage uniformly through FeatureSystem

**Feature Lifecycle Callbacks**:
- `OnInitialize()`: Feature initialization
- `OnBind()`: Bind client
- `OnAppExit()` / `OnFeatureExit()`: Application or Feature exit
- `OnAppInstall()` / `OnAppUninstall()` / `OnAppUpdate()`: Application lifecycle
- `OnUserChange()`: User switch
- `Online()` / `Offline()` / `OnReady()`: Device online/offline
- `OnSessionReady()`: Session ready
- `OnScreenUnlocked()`: Screen unlocked

### 4. StoreMetaData (Database Metadata)
- Contains metadata for **a single database** (appId, storeId, user, deviceId, instanceId, isEncrypt, etc.)
- Not a system managing all storage types, just describes configuration and state of a single database
- Used for server-side metadata management and sync coordination

### 5. Client-Server Interaction Pattern
- **Initial Connection**: Client → App Layer (FeatureStubImpl) → FeatureSystem → Feature
- **Subsequent Interaction**: Client ←→ Feature (direct interaction)
- **Client Death**: Client death → App Layer listens → FeatureSystem notifies Feature → Clean up resources

### 6. Dependency Injection Mechanism
- **Framework layer defines interfaces**: e.g., AccountDelegate, DeviceManagerDelegate
- **Adapters implement interfaces**: Implement interfaces and call system services
- **Adapters register to framework layer**: Register implementations during initialization
- **Service layer uses**: Use capabilities through framework layer interfaces, not depend on specific implementations

## Mandatory Requirements (MUST)

### Architecture Layering
- **Framework layer must not directly call system services**: Framework layer only defines interfaces, system service calls done by adapters
- **App layer must not directly depend on feature implementations**: App layer not aware of feature implementations
- **Service layer features must not depend on each other**: Feature implementations must not depend on other features, each feature can be independently configured to not compile

### Code Quality
- **Follow dependency injection pattern**: Service layer uses capabilities through framework layer interfaces, not depend on specific implementations
- **Correct namespaces**: Use correct namespaces (framework layer and common modules use `OHOS::DistributedData`)

### License
- **All source files must include Apache 2.0 license header**: C/C++/H files and Rust files have different formats
- **Copyright year must be the current year**: When creating new files, use the current year (e.g., 2026) in the copyright notice

### Code Style
- **Follow .clang-format configuration**: Access modifier offset -4, function parameters not aligned, short functions/loops not allowed on single line
- **Follow rustfmt.toml configuration**: Rust 2021 edition

### Performance
- **Use executors for async operations, do not start independent threads**: Use ExecutorPool for async operations

### Usage Details
- **Use AutoCache uniformly to acquire database handles**: Server-side must not use client to open databases
- **Do not retain handles acquired from AutoCache for long**: Server-acquired handles need to be closed promptly, cannot be retained for long
- **Use Serializable uniformly for serialization and deserialization**: Do not use external dependencies for serialization and deserialization

### Testing
- **Modifications and new code must have UT coverage**: Every modification must have corresponding test coverage
- **New external interfaces must have FUZZ test coverage**: External interfaces need FUZZ test coverage to prevent malicious input
- **Prohibit directly testing private methods**: Should treat as implementation details, cover through public interfaces. If not possible, consider optimizing code testability
- **Use unified mock classes when mocking**: Cases needing different implementations can inherit unified mock class to implement independent logic, common logic goes in mock class, avoid too much duplicate mock code

## Recommended Rules (SHOULD)

### Development Guidelines
- **Prioritize abstractions provided by framework layer**: e.g., AccountDelegate, DeviceManagerDelegate, EventCenter
- **Use smart pointers for lifecycle management**: Avoid memory leaks
- **Do not wait for other async tasks when using executors**: Avoid occupying thread time too long and deadlocks
- **Avoid capturing this in async operations**: Avoid UAF
- **Avoid cross-layer direct calls**: Upper layer calls lower layer, lower layer callbacks through interfaces
- **Extract common dependencies or tools as common classes or methods**: e.g., events shareable by features can be extracted to app layer for unified distribution, no need for each feature to perceive independently; tools shareable by app and service layer can be placed in framework layer

### Performance Optimization
- **Server-side implement flow control**: Prevent DOS
- **Use async IPC for IPC callbacks if return value not needed**: Reduce long-time occupation of IPC threads

### Error Handling
- **Adapters responsible for handling system service call exceptions**: Encapsulate and convert to framework layer interfaces
- **Use unified error codes**: Refer to error codes defined by framework layer

### Testing
- **UT tests should follow FIRST principles**: Fast, Independent, Repeatable, Self-Validating, Timely
- **Test names should clearly express intent**: Recommended format: TestedMethod_TestScenario_ExpectedResult
- **Organize tests using "Arrange-Act-Assert" pattern**: Clear test results
- **Use GMock objects to isolate dependencies**: Prioritize GMOCK

## Code Examples

### Correct: Use Framework Layer Interface

```cpp
// ✅ Correct: Acquire capabilities through framework layer interface
auto &delegate = AccountDelegate::GetInstance();
delegate->GetAccount();
```

### Wrong: Directly Call Adapter

```cpp
// ❌ Wrong: Directly depend on adapter layer implementation
auto adapter = std::make_shared<AccountAdapterImpl>();
adapter->GetAccount();
```

### Correct: Use AutoCache

```cpp
// ✅ Correct: Acquire handle through AutoCache
auto &cache = AutoCache::GetInstance();
auto [status, store] = cache.GetDBStore(metaData, {});
store->Insert("table_name", std::move(value));
// Handle will be automatically reclaimed, do not retain for long
```

### Wrong: Retain Handle for Long

```cpp
// ❌ Wrong: Retain database handle for long
class Service {
    std::shared_ptr<GeneralStore> store_; // Do not retain for long
};
```

### Event System

```cpp
auto &center = EventCenter::GetInstance();
center.Subscribe(evtId, [](const Event &event) { /* Handle event */ });
center.PostEvent(std::make_unique<Event>(evtId));
center.Unsubscribe(evtId);
```

## Build System

### Build Tool: GN (Generate Ninja)

Project uses OpenHarmony's GN-based build system, BUILD.gn files define build targets.

### Quick Build

```bash
# Build all data service modules
./build.sh --product-name <product> --build-target datamgr_service

# Fast build (if no gn files modified)
./build.sh --product-name <product> --build-target datamgr_service --fast-rebuild

# Build specific layer
./build.sh --product-name <product> --build-target //foundation/distributeddatamgr/datamgr_service/services/distributeddataservice/service:build_module
./build.sh --product-name <product> --build-target //foundation/distributeddatamgr/datamgr_service/services/distributeddataservice/framework:build_module
./build.sh --product-name <product> --build-target //foundation/distributeddatamgr/datamgr_service/services/distributeddataservice/app:build_module

# Build all unit tests
./build.sh --product-name <product> --build-target datamgr_service_test

```

### Feature Switches

Controlled in `datamgr_service.gni`:
```gni
datamgr_service_cloud = true         # Cloud sync support (default enabled)
datamgr_service_rdb = true           # Relational database support (default enabled)
datamgr_service_kvdb = true          # Key-value database support (default enabled)
datamgr_service_object = true        # Object storage support (default enabled)
datamgr_service_data_share = true    # Data sharing support (default enabled)
datamgr_service_udmf = false         # UDMF service (default disabled)
```

## Directory Structure

```
services/distributeddataservice/
├── adapter/                  # Adapters (part of service layer)
│   ├── account/              # Account system adapter
│   ├── communicator/         # Communication adapter
│   ├── dfx/                  # Observability adapter
│   ├── network/              # Network adapter
│   ├── screenlock/           # Screen lock adapter
│   └── utils/                # Utilities
├── app/                      # App Layer
│   └── src/                  # Source code
├── framework/                # Framework Layer
│   ├── include/              # Public API
│   └── <component>/          # Component implementations
├── service/                  # Service Layer
│   ├── rdb/                  # RDB Feature
│   ├── kvdb/                 # KVDB Feature
│   ├── cloud/                # Cloud Feature
│   ├── object/               # Object Feature
│   ├── data_share/           # DataShare Feature
│   ├── udmf/                 # UDMF Feature
│   └── permission/           # Permission check
└── rust/                     # Rust extensions
    └── ylong_cloud_extension/
```

## Common Issues

- **AutoCache handle leak**: Do not retain handles acquired from AutoCache as member variables
- **Feature not found**: Check if feature is enabled in `datamgr_service.gni` and correctly registered
- **Build errors**: Ensure feature flags are consistent between module dependencies
- **Circular dependencies**: Service layer features must not depend on each other
- **'log_print.h' file not found**: Ensure `external_deps = [ "kv_store:datamgr_common" ]` is added in BUILD.gn to access logging macros (ZLOGD, ZLOGE, etc.)

## Reference Documentation

- OpenHarmony Architecture Overview: https://gitcode.com/openharmony/docs/blob/master/zh-cn/readme/分布式数据管理子系统.md
- Application Development Guide: https://gitcode.com/openharmony/docs/tree/master/zh-cn/application-dev/database
- Project README: [README.md](README.md) / [README_zh.md](README_zh.md)

## Summary

When developing in this codebase, you must:
1. **Understand architecture separation**: Clearly define boundaries between client and server responsibilities
2. **Understand three-layer architecture**: App layer, service layer (including adapters), framework layer
3. **Understand key concepts**: GeneralStore (server handle), AutoCache (handle management), FeatureSystem (feature management), StoreMetaData (metadata)
4. **Follow dependency injection**: Service layer acquires capabilities through framework layer interfaces
5. **Follow mandatory requirements**: Architecture layering, code quality, license, code style, performance, testing
6. **Ensure test coverage**: Modifications and new code must have UT coverage, external interfaces must have FUZZ test coverage
