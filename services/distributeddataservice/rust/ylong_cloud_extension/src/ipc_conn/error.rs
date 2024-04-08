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

use std::fmt::{Debug, Display, Formatter};

use ipc::parcel::{Deserialize, MsgParcel};
use ipc::{IpcResult, IpcStatusCode};

use crate::ipc_conn::vec_raw_read;

#[derive(Default, Debug)]
pub(crate) struct Errors(pub(crate) Vec<Error>);

impl Deserialize for Errors {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = Errors(vec_raw_read::<Error>(parcel)?);
        Ok(result)
    }
}

#[derive(Debug, Eq, PartialEq, Clone)]
/// Indicates IPC connection and data acquisition errors.
pub enum Error {
    /// Successful.
    Success,

    /// Creates MsgParcel Failed.
    CreateMsgParcelFailed,

    /// Writes MsgParcel Failed.
    WriteMsgParcelFailed,

    /// Reads MsgParcel Failed.
    ReadMsgParcelFailed,

    /// Gets Proxy Object Failed.
    GetProxyObjectFailed,

    /// Sends Request Failed.
    SendRequestFailed,

    /// Unlock Failed.
    UnlockFailed,

    /// Invalid Cloud Status.
    InvalidCloudStatus,

    /// Invalid Space Status.
    InvalidSpaceStatus,

    /// Downloads Failed.
    DownloadFailed,

    /// Uploads Failed.
    UploadFailed,

    /// Invalid Field Type Code
    InvalidFieldType,

    /// Unknown error.
    UnknownError,

    /// Network error.
    NetworkError,

    /// Cloud is disabled.
    CloudDisabled,

    /// The cloud database is locked by others.
    LockedByOthers,

    /// The number of records exceeds the limit.
    RecordLimitExceeded,

    /// The cloud has no space for the asset.
    NoSpaceForAsset,
}

impl Display for Error {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::Success => write!(f, "Successful"),
            Error::CreateMsgParcelFailed => write!(f, "Creates MsgParcel Failed"),
            Error::WriteMsgParcelFailed => write!(f, "Writes MsgParcel Failed"),
            Error::ReadMsgParcelFailed => write!(f, "Reads MsgParcel Failed"),
            Error::GetProxyObjectFailed => write!(f, "Gets Proxy Object Failed"),
            Error::SendRequestFailed => write!(f, "Sends Request Failed"),
            Error::UnlockFailed => write!(f, "Unlock failed"),
            Error::InvalidCloudStatus => write!(f, "Invalid Cloud Status"),
            Error::InvalidSpaceStatus => write!(f, "Invalid Space Status"),
            Error::DownloadFailed => write!(f, "Downloads Failed"),
            Error::UploadFailed => write!(f, "Uploads Failed"),
            Error::InvalidFieldType => write!(f, "Invalid Field Type Code"),
            Error::UnknownError => write!(f, "Unknown error"),
            Error::NetworkError => write!(f, "Network error"),
            Error::CloudDisabled => write!(f, "Cloud is disabled"),
            Error::LockedByOthers => write!(f, "The cloud database is locked by others"),
            Error::RecordLimitExceeded => write!(f, "The number of records exceeds the limit"),
            Error::NoSpaceForAsset => write!(f, "The cloud has no space for the asset"),
        }
    }
}

impl std::error::Error for Error {}

impl Deserialize for Error {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let index = parcel.read::<u32>()?;
        match index {
            0 => Ok(Error::Success),
            1 => Ok(Error::CreateMsgParcelFailed),
            2 => Ok(Error::WriteMsgParcelFailed),
            3 => Ok(Error::ReadMsgParcelFailed),
            4 => Ok(Error::GetProxyObjectFailed),
            5 => Ok(Error::SendRequestFailed),
            6 => Ok(Error::InvalidCloudStatus),
            7 => Ok(Error::InvalidSpaceStatus),
            8 => Ok(Error::DownloadFailed),
            9 => Ok(Error::UploadFailed),
            10 => Ok(Error::InvalidFieldType),
            11 => Ok(Error::UnknownError),
            12 => Ok(Error::NetworkError),
            13 => Ok(Error::CloudDisabled),
            14 => Ok(Error::LockedByOthers),
            15 => Ok(Error::RecordLimitExceeded),
            16 => Ok(Error::NoSpaceForAsset),
            _ => Err(IpcStatusCode::Failed),
        }
    }
}
