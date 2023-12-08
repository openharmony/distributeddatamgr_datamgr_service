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

use crate::ipc_conn;
use std::fmt::{Debug, Display, Formatter};

/// Struct of Synchronization Error.
pub enum SyncError {
    /// Unknown error
    Unknown,

    /// Target table is not in the source database
    NoSuchTableInDb,

    /// Error because the input is not an asset value and can't call asset relating functions
    NotAnAsset,

    /// General asset download failure
    AssetDownloadFailure,

    /// General asset upload failure
    AssetUploadFailure,

    /// Session has been unlocked and can't be unclocked again
    SessionUnlocked,

    /// Unsupported functions or features
    Unsupported,

    /// IPCError
    IPCError(ipc_conn::Error),

    /// A bunch of IPC Errors
    IPCErrors(Vec<ipc_conn::Error>),

    /// IO Error
    IO(std::io::Error),
}

impl Clone for SyncError {
    fn clone(&self) -> Self {
        match self {
            SyncError::Unknown => SyncError::Unknown,
            SyncError::NoSuchTableInDb => SyncError::NoSuchTableInDb,
            SyncError::NotAnAsset => SyncError::NotAnAsset,
            SyncError::AssetDownloadFailure => SyncError::AssetDownloadFailure,
            SyncError::AssetUploadFailure => SyncError::AssetUploadFailure,
            SyncError::SessionUnlocked => SyncError::SessionUnlocked,
            SyncError::Unsupported => SyncError::Unsupported,
            SyncError::IPCError(a) => SyncError::IPCError(a.clone()),
            SyncError::IPCErrors(a) => SyncError::IPCErrors(a.clone()),
            SyncError::IO(a) => {
                let e = std::io::Error::from(a.kind());
                SyncError::IO(e)
            }
        }
    }
}

impl From<std::io::Error> for SyncError {
    fn from(value: std::io::Error) -> Self {
        SyncError::IO(value)
    }
}

impl From<ipc_conn::Error> for SyncError {
    fn from(value: ipc_conn::Error) -> Self {
        SyncError::IPCError(value)
    }
}

impl From<ipc_conn::Errors> for SyncError {
    fn from(value: ipc_conn::Errors) -> Self {
        SyncError::IPCErrors(value.0)
    }
}

impl Debug for SyncError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let str = match self {
            SyncError::Unknown => "unknown".to_string(),
            SyncError::NotAnAsset => "NotAnAsset".to_string(),
            SyncError::AssetDownloadFailure => "AssetDownloadFailure".to_string(),
            SyncError::AssetUploadFailure => "AssetUploadFailure".to_string(),
            SyncError::NoSuchTableInDb => "NoSuchTableInDb".to_string(),
            SyncError::SessionUnlocked => "SessionUnlocked".to_string(),
            SyncError::Unsupported => "Unsupported".to_string(),
            SyncError::IPCError(e) => e.to_string(),
            SyncError::IPCErrors(es) => {
                let mut ret = String::new();
                for (idx, e) in es.iter().enumerate() {
                    ret.push_str(&format!("\n\tidx {}: {}", idx, e));
                }
                ret
            }
            SyncError::IO(e) => e.to_string(),
        };
        write!(f, "{}", str)
    }
}

impl Display for SyncError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl std::error::Error for SyncError {}
