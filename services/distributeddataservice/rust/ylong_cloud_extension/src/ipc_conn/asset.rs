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

use crate::ipc_conn::error::Error;
use crate::ipc_conn::ffi::ConnectService;
use crate::ipc_conn::function::AssetLoaderFunc::{Download, Upload};
use crate::ipc_conn::function::CloudServiceFunc::ConnectAssetLoader;
use crate::ipc_conn::{vec_raw_read, vec_raw_write, AssetStatus};
use ipc_rust::{
    BorrowedMsgParcel, Deserialize, IRemoteObj, MsgParcel, RemoteObj, Serialize, String16,
};

pub(crate) type AssetLoaderResult<T> = Result<T, Error>;

/// CloudAsset struct storing relating information to upload and download assets.
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
    fn serialize(&self, parcel: &mut BorrowedMsgParcel<'_>) -> ipc_rust::IpcResult<()> {
        let asset_name = String16::new(&self.asset_name);
        let uri = String16::new(&self.uri);
        let sub_path = String16::new(&self.sub_path);
        let create_time = String16::new(&self.create_time);
        let modify_time = String16::new(&self.modify_time);
        let size = String16::new(&self.size);
        let asset_id = String16::new(&self.asset_id);
        let hash = String16::new(&self.hash);

        parcel.write(&asset_name)?;
        parcel.write(&uri)?;
        parcel.write(&sub_path)?;
        parcel.write(&create_time)?;
        parcel.write(&modify_time)?;
        parcel.write(&size)?;
        parcel.write(&self.status)?;
        parcel.write(&asset_id)?;
        parcel.write(&hash)?;
        Ok(())
    }
}

impl Deserialize for CloudAsset {
    fn deserialize(parcel: &BorrowedMsgParcel<'_>) -> ipc_rust::IpcResult<Self> {
        let asset_name = parcel.read::<String16>()?;
        let uri = parcel.read::<String16>()?;
        let sub_path = parcel.read::<String16>()?;
        let create_time = parcel.read::<String16>()?;
        let modify_time = parcel.read::<String16>()?;
        let size = parcel.read::<String16>()?;
        let operation_type = parcel.read::<AssetStatus>()?;
        let asset_id = parcel.read::<String16>()?;
        let hash = parcel.read::<String16>()?;

        let result = CloudAsset {
            asset_id: asset_id.get_string(),
            asset_name: asset_name.get_string(),
            hash: hash.get_string(),
            uri: uri.get_string(),
            sub_path: sub_path.get_string(),
            create_time: create_time.get_string(),
            modify_time: modify_time.get_string(),
            size: size.get_string(),
            status: operation_type,
        };
        Ok(result)
    }
}

/// CloudAssets.
#[derive(Default, PartialEq, Debug, Clone)]
pub struct CloudAssets(pub Vec<CloudAsset>);

impl Serialize for CloudAssets {
    fn serialize(&self, parcel: &mut BorrowedMsgParcel<'_>) -> ipc_rust::IpcResult<()> {
        vec_raw_write(parcel, &self.0)
    }
}

impl Deserialize for CloudAssets {
    fn deserialize(parcel: &BorrowedMsgParcel<'_>) -> ipc_rust::IpcResult<Self> {
        let result = CloudAssets(vec_raw_read::<CloudAsset>(parcel)?);
        Ok(result)
    }
}

pub(crate) struct AssetLoader {
    pub(crate) remote_obj: Option<RemoteObj>,
}

impl AssetLoader {
    pub(crate) fn new(user_id: i32) -> AssetLoaderResult<Self> {
        let msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let function_number = ConnectAssetLoader as u32;
        let remote_obj = unsafe { RemoteObj::from_raw_ciremoteobj(ConnectService(user_id)) }
            .ok_or(Error::GetProxyObjectFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let result = receive
            .read::<RemoteObj>()
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
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let table = String16::new(table);
        let gid = String16::new(gid);
        let prefix = String16::new(prefix);
        msg_parcel
            .write(&table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(&gid)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(&prefix)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(assets)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Download as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::GetProxyObjectFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
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
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let table = String16::new(table);
        let gid = String16::new(gid);
        let prefix = String16::new(prefix);
        msg_parcel
            .write(&table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(&gid)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(&prefix)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(assets)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Upload as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::GetProxyObjectFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
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

#[cfg(test)]
#[cfg(feature = "test_server_ready")]
mod asset_server_ready {
    use crate::ipc_conn::asset::AssetLoader;
    use crate::ipc_conn::{AssetStatus, CloudAsset, CloudAssets};
    use std::time::SystemTime;

    /// UT test for asset_new.
    ///
    /// # Title
    /// ut_asset_new
    ///
    /// # Brief
    /// 1. Create a `AssetLoader` struct.
    /// 2. Get self info.
    /// 3. Check if it is correct.
    #[test]
    fn ut_asset_loader_new() {
        let user_id = 100;
        let asset = AssetLoader::new(user_id).unwrap();
        assert!(asset.remote_obj.is_some());
    }

    /// UT test for asset_new.
    ///
    /// # Title
    /// ut_asset_new
    ///
    /// # Brief
    /// 1. Create a `AssetLoader` struct.
    /// 2. Download info.
    /// 3. Check if it is correct.
    #[test]
    fn ut_asset_loader_download() {
        let user_id = 100;
        let asset = AssetLoader::new(user_id).unwrap();
        let mut assets = CloudAssets::default();

        let instant = SystemTime::now()
            .duration_since(SystemTime::UNIX_EPOCH)
            .unwrap()
            .as_secs()
            .to_string();
        let asset_one = CloudAsset {
            asset_id: "".to_string(),
            asset_name: "whj_1.txt".to_string(),
            hash: "".to_string(),
            uri: "file:://com.huawei.hmos.notepad/data/storage/el2/distributedfiles/dir/whj_1.txt"
                .to_string(),
            sub_path: "12345_whj".to_string(),
            create_time: instant.clone(),
            modify_time: instant.clone(),
            size: "2508026".to_string(),
            status: AssetStatus::Insert,
        };
        let asset_two = CloudAsset {
            asset_id: "".to_string(),
            asset_name: "whj_2.txt".to_string(),
            hash: "".to_string(),
            uri: "file:://com.huawei.hmos.notepad/data/storage/el2/distributedfiles/dir/whj_2.txt"
                .to_string(),
            sub_path: "12345_whj_2".to_string(),
            create_time: instant.clone(),
            modify_time: instant,
            size: "6458562".to_string(),
            status: AssetStatus::Insert,
        };
        assets.0.push(asset_one);
        assets.0.push(asset_two);

        let table = "cloud_notes";
        let gid = "Fh-N0N6T454fidsh9ds1KcJdUb5wVeicH";
        let prefix = "cb100ed1$96e$431e$8b35$8a5d96056a71";
        let result = asset.download(table, gid, prefix, &assets);
        assert!(result.is_ok());
    }

    /// UT test for asset_new.
    ///
    /// # Title
    /// ut_asset_new
    ///
    /// # Brief
    /// 1. Create a `AssetLoader` struct.
    /// 2. Upload info.
    /// 3. Check if it is correct.
    #[test]
    fn ut_asset_loader_upload() {
        let user_id = 100;
        let asset = AssetLoader::new(user_id).unwrap();
        let mut assets = CloudAssets::default();

        let instant = SystemTime::now()
            .duration_since(SystemTime::UNIX_EPOCH)
            .unwrap()
            .as_secs()
            .to_string();
        let asset_one = CloudAsset {
            asset_id: "".to_string(),
            asset_name: "whj_1.txt".to_string(),
            hash: "".to_string(),
            uri: "file:://com.huawei.hmos.notepad/data/storage/el2/distributedfiles/dir/whj_1.txt"
                .to_string(),
            sub_path: "12345_whj".to_string(),
            create_time: instant.clone(),
            modify_time: instant.clone(),
            size: "2508026".to_string(),
            status: AssetStatus::Insert,
        };
        let asset_two = CloudAsset {
            asset_id: "".to_string(),
            asset_name: "whj_2.txt".to_string(),
            hash: "".to_string(),
            uri: "file:://com.huawei.hmos.notepad/data/storage/el2/distributedfiles/dir/whj_2.txt"
                .to_string(),
            sub_path: "12345_whj_2".to_string(),
            create_time: instant.clone(),
            modify_time: instant,
            size: "6458562".to_string(),
            status: AssetStatus::Insert,
        };
        assets.0.push(asset_one);
        assets.0.push(asset_two);

        let table = "cloud_notes";
        let gid = "Fh-N0N6T454fidsh9ds1KcJdUb5wVeicH";
        let prefix = "cb100ed1$96e$431e$8b35$8a5d96056a71";
        let result = asset.upload(table, gid, prefix, &assets);
        assert!(result.is_ok());
    }
}