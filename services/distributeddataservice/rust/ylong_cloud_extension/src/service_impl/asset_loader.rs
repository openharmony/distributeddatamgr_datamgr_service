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

pub use crate::ipc_conn::{AssetStatus, CloudAsset};
use crate::service_impl::error::SyncError;
use crate::service_impl::types::{Database, Table};
use crate::{ipc_conn, SyncResult};
use std::collections::HashMap;

/// Cloud Asset loader struct.
pub struct CloudAssetLoader<'a> {
    asset_loader: ipc_conn::AssetLoader,
    tables: &'a HashMap<String, Table>,
    // Keep the following fields below for future extension.
    #[allow(unused)]
    bundle_name: String,
}

impl<'a> CloudAssetLoader<'a> {
    /// Initialize a CloudAssetLoader instance from user id, bundle name, and database.
    /// Database can be obtained from CloudInfo and CloudSync.
    pub fn new(user_id: i32, bundle_name: &str, db: &'a Database) -> SyncResult<Self> {
        let asset_loader = ipc_conn::AssetLoader::new(user_id)?;
        Ok(CloudAssetLoader {
            asset_loader,
            tables: &db.tables,
            bundle_name: bundle_name.to_string(),
        })
    }

    /// Take in table name, gid, prefix, and assets, send upload request to the other side of IPC
    /// connection. Necessary information will be updated in the parameter assets. The cloud storage
    /// will ask for files from source paths, and this function only returns result and relating infos.
    pub fn upload(
        &self,
        table_name: &str,
        gid: &str,
        prefix: &str,
        assets: &[CloudAsset],
    ) -> SyncResult<Vec<Result<CloudAsset, SyncError>>> {
        self.upload_download_inner(
            table_name,
            gid,
            prefix,
            assets,
            ipc_conn::AssetLoader::upload,
        )
    }

    /// Take in table name, gid, prefix, and assets, send download request to the other side of IPC
    /// connection. Necessary information will be updated in the parameter assets. The cloud storage
    /// will send files to target paths, and this function only returns result and relating infos.
    pub fn download(
        &self,
        table_name: &str,
        gid: &str,
        prefix: &str,
        assets: &[CloudAsset],
    ) -> SyncResult<Vec<Result<CloudAsset, SyncError>>> {
        self.upload_download_inner(
            table_name,
            gid,
            prefix,
            assets,
            ipc_conn::AssetLoader::download,
        )
    }

    /// Remove local file according to the path in the asset passed in.
    pub fn remove_local_assets(asset: &CloudAsset) -> SyncResult<()> {
        std::fs::remove_file(asset.local_path())?;
        Ok(())
    }

    fn upload_download_inner<F>(
        &self,
        table_name: &str,
        gid: &str,
        prefix: &str,
        assets: &[CloudAsset],
        mut f: F,
    ) -> SyncResult<Vec<Result<CloudAsset, SyncError>>>
    where
        F: FnMut(
            &ipc_conn::AssetLoader,
            &str,
            &str,
            &str,
            &ipc_conn::CloudAssets,
        ) -> Result<Vec<Result<CloudAsset, ipc_conn::Error>>, ipc_conn::Error>,
    {
        let mut assets_ipc = ipc_conn::CloudAssets::default();
        for asset in assets.iter() {
            if asset.status == AssetStatus::Delete {
                CloudAssetLoader::remove_local_assets(asset)?;
            }

            assets_ipc.0.push(asset.clone());
        }
        let alias = self.get_table_alias(table_name);
        let ret = f(&self.asset_loader, alias, gid, prefix, &assets_ipc)?;
        let mut ret_map = vec![];
        for single_result in ret {
            ret_map.push(single_result.map_err(|e| e.into()));
        }
        Ok(ret_map)
    }

    fn get_table_alias(&self, table_name: &str) -> &str {
        match self.tables.get(table_name) {
            None => "",
            Some(t) => t.alias.as_str(),
        }
    }
}
