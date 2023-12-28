/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! Cloud Service Synchronization Crate, implemented in Rust. This crate is used as bridge between
//! upper C++ apk and JS cloud service. C++ will call C-ffi of Rust, and Rust will use IPC to
//! connect to JS.

/// Module of C FFI adapter.
///
/// # Attention
/// For pointers passed into functions as parameters, they should remain valid throughout the
/// lifetime of callees, or memory issues and undefined behavior might happen.
///
/// Besides, for String parameters, users need to guarantee that the source char array doesn't
/// contain non-UTF8 literals, and the length passed in is valid.
pub mod c_adapter;

/// Module of IPC Conn Serialization and Deserialization.
pub mod ipc_conn;

/// Module of Cloud Service Synchronization, providing structs of CloudAssetLoader, CloudDatabase,
/// CloudSync and relating structs and functions.
pub mod service_impl;

use crate::service_impl::error::SyncError;

/// Sync Result returned by CloudAssetLoader, CloudDatabase, and CloudSync.
pub type SyncResult<T> = Result<T, SyncError>;
