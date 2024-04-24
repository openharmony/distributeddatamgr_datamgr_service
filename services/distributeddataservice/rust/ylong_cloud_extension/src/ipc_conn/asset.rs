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

use ipc::parcel::{Deserialize, MsgParcel, Serialize};
use ipc::remote::RemoteObj;
use ipc::IpcResult;

use crate::ipc_conn::error::Error;
use crate::ipc_conn::ffi::ConnectService;
use crate::ipc_conn::function::AssetLoaderFunc::{Download, Upload};
use crate::ipc_conn::function::CloudServiceFunc::ConnectAssetLoader;
use crate::ipc_conn::{vec_raw_read, vec_raw_write, AssetStatus};
pub(crate) type AssetLoaderResult<T> = Result<T, Error>;

/// CloudAsset struct storing relating information to upload and download
/// assets.
#[derive(Default, PartialEq, Clone, Debug)]
pub struct CloudAsset {
    pub(crate) asset_id: String,
    pub(crate) asset_name: String,
    pub(crate) hash: String,
    pub(crate) uri: String,
    pub(crate) sub_path: String,
    pub(crate) create_time: String,
    pub(crate) modify_time: String,
    pub(crate) size: String,
    pub(crate) status: AssetStatus,
}

impl CloudAsset {
    /// Get CloudAsset id.
    pub fn id(&self) -> &str {
        &self.asset_id
    }

    /// Get CloudAsset name.
    pub fn name(&self) -> &str {
        &self.asset_name
    }

    /// Get CloudAsset uri, a real path pointing to the CloudAsset.
    pub fn uri(&self) -> &str {
        &self.uri
    }

    /// Get CloudAsset local path relative to the application sandbox.
    pub fn local_path(&self) -> &str {
        &self.sub_path
    }

    /// Get create time of this CloudAsset.
    pub fn create_time(&self) -> &str {
        &self.create_time
    }

    /// Get modify time of this CloudAsset.
    pub fn modify_time(&self) -> &str {
        &self.modify_time
    }

    /// Get size of this CloudAsset in kb.
    pub fn size(&self) -> &str {
        &self.size
    }

    /// Get hash value of this CloudAsset.
    pub fn hash(&self) -> &str {
        &self.hash
    }

    /// Set CloudAsset id.
    pub fn set_id(&mut self, id: String) {
        self.asset_id = id;
    }

    /// Set CloudAsset name.
    pub fn set_name(&mut self, name: String) {
        self.asset_name = name;
    }

    /// Set CloudAsset uri, a real path pointing to the CloudAsset.
    pub fn set_uri(&mut self, uri: String) {
        self.uri = uri;
    }

    /// Set CloudAsset local path relative to the application sandbox.
    pub fn set_local_path(&mut self, local_path: String) {
        self.sub_path = local_path;
    }

    /// Set create time of this CloudAsset.
    pub fn set_create_time(&mut self, create_time: String) {
        self.create_time = create_time;
    }

    /// Set modify time of this CloudAsset.
    pub fn set_modify_time(&mut self, modify_time: String) {
        self.modify_time = modify_time;
    }

    /// Set size of this CloudAsset in kb.
    pub fn set_size(&mut self, size: String) {
        self.size = size;
    }

    /// Set hash value of this CloudAsset.
    pub fn set_hash(&mut self, hash: String) {
        self.hash = hash;
    }
}

impl Serialize for CloudAsset {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        parcel.write_string16(&self.asset_name)?;
        parcel.write_string16(&self.uri)?;
        parcel.write_string16(&self.sub_path)?;
        parcel.write_string16(&self.create_time)?;
        parcel.write_string16(&self.modify_time)?;
        parcel.write_string16(&self.size)?;
        parcel.write(&self.status)?;
        parcel.write_string16(&self.asset_id)?;
        parcel.write_string16(&self.hash)?;
        Ok(())
    }
}

impl Deserialize for CloudAsset {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let asset_name = parcel.read_string16()?;
        let uri = parcel.read_string16()?;
        let sub_path = parcel.read_string16()?;
        let create_time = parcel.read_string16()?;
        let modify_time = parcel.read_string16()?;
        let size = parcel.read_string16()?;
        let operation_type = parcel.read::<AssetStatus>()?;
        let asset_id = parcel.read_string16()?;
        let hash = parcel.read_string16()?;

        let result = CloudAsset {
            asset_id,
            asset_name,
            hash,
            uri,
            sub_path,
            create_time,
            modify_time,
            size,
            status: operation_type,
        };
        Ok(result)
    }
}

/// CloudAssets.
#[derive(Default, PartialEq, Debug, Clone)]
pub struct CloudAssets(pub Vec<CloudAsset>);

impl Serialize for CloudAssets {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        vec_raw_write(parcel, &self.0)
    }
}

impl Deserialize for CloudAssets {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = CloudAssets(vec_raw_read::<CloudAsset>(parcel)?);
        Ok(result)
    }
}

pub(crate) struct AssetLoader {
    pub(crate) remote_obj: Option<RemoteObj>,
}

impl AssetLoader {
    pub(crate) fn new(user_id: i32) -> AssetLoaderResult<Self> {
        let mut msg_parcel = MsgParcel::new();
        let function_number = ConnectAssetLoader as u32;
        let remote_obj = unsafe { RemoteObj::from_ciremote(ConnectService(user_id)) }
            .ok_or(Error::GetProxyObjectFailed)?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Error::SendRequestFailed)?;

        let result = receive
            .read_remote()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        Ok(Self {
            remote_obj: Some(result),
        })
    }

    // Responsible for telling the cloud to perform file downloads.
    pub(crate) fn download(
        &self,
        table: &str,
        gid: &str,
        prefix: &str,
        assets: &CloudAssets,
    ) -> AssetLoaderResult<Vec<Result<CloudAsset, Error>>> {
        let mut msg_parcel = MsgParcel::new();
        msg_parcel
            .write_string16(table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write_string16(gid)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write_string16(prefix)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(assets)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Download as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::GetProxyObjectFailed)?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Error::SendRequestFailed)?;

        let mut results = vec![];
        // Deserialise the length of the returned result.
        let length = receive
            .read::<i32>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        for _ in 0..length {
            let error = receive
                .read::<Error>()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            if !error.eq(&Error::Success) {
                results.push(Err(error));
            } else {
                let asset = receive
                    .read::<CloudAsset>()
                    .map_err(|_| Error::ReadMsgParcelFailed)?;
                results.push(Ok(asset))
            }
        }
        Ok(results)
    }

    // Responsible for telling the cloud to perform file uploads.
    pub(crate) fn upload(
        &self,
        table: &str,
        gid: &str,
        prefix: &str,
        assets: &CloudAssets,
    ) -> AssetLoaderResult<Vec<Result<CloudAsset, Error>>> {
        let mut msg_parcel = MsgParcel::new();
        msg_parcel
            .write_string16(table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write_string16(gid)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write_string16(prefix)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(assets)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Upload as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::GetProxyObjectFailed)?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Error::SendRequestFailed)?;

        let mut results = vec![];
        // Deserialise the length of the returned result.
        let length = receive
            .read::<i32>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        for _ in 0..length {
            let error = receive
                .read::<Error>()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            if !error.eq(&Error::Success) {
                results.push(Err(error));
            } else {
                let asset = receive
                    .read::<CloudAsset>()
                    .map_err(|_| Error::ReadMsgParcelFailed)?;
                results.push(Ok(asset))
            }
        }
        Ok(results)
    }
}
