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

use crate::service_impl::types::Database;
use crate::{ipc_conn, SyncResult};
use std::collections::HashMap;
use std::ops::Deref;

/// Struct of CloudSync
pub struct CloudSync {
    user_id: i32,
    connect: ipc_conn::Connect,
}

impl std::fmt::Debug for CloudSync {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "CloudSync: user_id = {:?}", self.user_id)
    }
}

impl CloudSync {
    /// Get a server instance through IPC, and initialize a CloudSync instance
    pub fn new(user_id: i32) -> SyncResult<Self> {
        Ok(CloudSync {
            user_id,
            connect: ipc_conn::Connect::new(user_id)?,
        })
    }

    #[inline]
    fn get_cloud_info(&mut self) -> SyncResult<CloudInfo> {
        let user_info_ipc = self.connect.get_service_info(self.user_id)?;
        Ok(CloudInfo::from(user_info_ipc.deref()))
    }

    /// Get a cloud server info from the other end through the IPC connection
    pub fn get_service_info(&mut self) -> SyncResult<CloudInfo> {
        let mut cloud_info = self.get_cloud_info()?;
        cloud_info.user = self.user_id;
        let app_infos = self.connect.get_app_brief_info(self.user_id)?;
        for app in &app_infos.0 {
            let cloud_app_info = AppInfo::from(app.1);
            cloud_info
                .apps
                .insert(cloud_app_info.bundle_name.clone(), cloud_app_info);
        }
        Ok(cloud_info)
    }

    /// Get a application schema by bundle name from the other end through the IPC connection
    pub fn get_app_schema(&mut self, bundle_name: &str) -> SyncResult<SchemaMeta> {
        let schema_ipc = self.connect.get_app_schema(self.user_id, bundle_name)?;
        Ok(SchemaMeta::from(schema_ipc.deref()))
    }

    /// Passing in relations and a callback function, if possible, send the subscription relation
    /// to the cloud server and the specific database.
    pub fn subscribe(
        &mut self,
        dbs: &HashMap<String, Vec<Database>>,
        expire: i64,
    ) -> SyncResult<HashMap<String, RelationSet>> {
        let mut result = HashMap::new();
        for (bundle_name, dbmetas) in dbs {
            let mut vec = vec![];
            for dbmeta in dbmetas {
                vec.push(dbmeta.try_into()?)
            }
            let dbs_ipc = ipc_conn::Databases(vec);

            let ret = self.connect.subscribe(expire, bundle_name, &dbs_ipc)?;
            let (ret_expire, map) = ret.0;
            for (bundle_name_ret, sub_result) in map {
                let mut r = RelationSet {
                    bundle_name: bundle_name.clone(),
                    expire_time: ret_expire,
                    relations: HashMap::new(),
                };
                for (alias, id) in sub_result.0 {
                    r.relations.insert(alias, id);
                }
                result.insert(bundle_name_ret, r);
            }
        }
        Ok(result)
    }

    /// Passing in relations and a callback function, if possible, send the unsubscription relation
    /// to the cloud server and the specific database.
    pub fn unsubscribe(&mut self, relations: &HashMap<String, Vec<String>>) -> SyncResult<()> {
        if relations.is_empty() {
            return Ok(());
        }
        let unsub = ipc_conn::UnsubscriptionInfo(relations.clone());
        self.connect.unsubscribe(&unsub)?;
        Ok(())
    }
}

/// Struct of SchemaMeta, containing schema information on a cloud server.
pub struct SchemaMeta {
    version: i32,
    bundle_name: String,
    databases: Vec<Database>,
}

impl Default for SchemaMeta {
    fn default() -> Self {
        SchemaMeta {
            version: 0,
            bundle_name: "".to_string(),
            databases: vec![],
        }
    }
}

impl SchemaMeta {
    /// Get version of SchemaMeta instance.
    pub fn version(&self) -> i32 {
        self.version
    }

    /// Get bundle name of SchemaMeta instance.
    pub fn bundle_name(&self) -> &str {
        &self.bundle_name
    }

    /// Get database from a SchemaMeta instance by store id.
    pub fn databases(&self) -> &[Database] {
        &self.databases
    }
}

impl From<&ipc_conn::Schema> for SchemaMeta {
    fn from(value: &ipc_conn::Schema) -> Self {
        let mut dbs = vec![];
        for db in &value.databases.0 {
            dbs.push(Database::from(db));
        }
        SchemaMeta {
            version: value.version,
            bundle_name: value.bundle_name.clone(),
            databases: dbs,
        }
    }
}

/// Struct of CloudInfo
pub struct CloudInfo {
    pub(crate) user: i32,
    pub(crate) id: String,
    pub(crate) total_space: u64,
    pub(crate) remain_space: u64,
    pub(crate) enable_cloud: bool,
    pub(crate) apps: HashMap<String, AppInfo>,
}

impl From<&ipc_conn::ServiceInfo> for CloudInfo {
    fn from(value: &ipc_conn::ServiceInfo) -> Self {
        CloudInfo {
            user: value.user,
            id: value.account_id.clone(),
            total_space: value.total_space,
            remain_space: value.remain_space,
            enable_cloud: value.enable_cloud,
            apps: Default::default(),
        }
    }
}

impl CloudInfo {
    /// Get user of CloudInfo instance.
    pub fn get_user(&self) -> i32 {
        self.user
    }

    /// Get id of CloudInfo instance.
    pub fn get_id(&self) -> &str {
        &self.id
    }

    /// Get total space of CloudInfo instance.
    pub fn get_total_space(&self) -> u64 {
        self.total_space
    }

    /// Get remain space of CloudInfo instance.
    pub fn get_remain_space(&self) -> u64 {
        self.remain_space
    }

    /// Check whether the CloudInfo instance enables cloud synchronization.
    pub fn get_enable_cloud(&self) -> bool {
        self.enable_cloud
    }

    /// Get app info from CloudInfo instance.
    pub fn get_app_info(&self) -> &HashMap<String, AppInfo> {
        &self.apps
    }
}

/// Struct of App Info.
#[derive(Clone)]
pub struct AppInfo {
    pub(crate) bundle_name: String,
    pub(crate) app_id: String,
    pub(crate) instance_id: i32,
    pub(crate) cloud_switch: bool,
}

impl From<&ipc_conn::AppInfo> for AppInfo {
    fn from(value: &ipc_conn::AppInfo) -> Self {
        AppInfo {
            bundle_name: value.bundle_name.clone(),
            app_id: value.app_id.clone(),
            instance_id: value.instance_id,
            cloud_switch: value.cloud_switch,
        }
    }
}

impl AppInfo {
    /// Get bundle name of AppInfo instance.
    pub fn bundle_name(&self) -> &str {
        &self.bundle_name
    }

    /// Get app id of AppInfo instance.
    pub fn app_id(&self) -> &str {
        &self.app_id
    }

    /// Check whether the AppInfo instance allows cloud switch.
    pub fn cloud_switch(&self) -> bool {
        self.cloud_switch
    }

    /// Get instance id of AppInfo instance.
    pub fn instance_id(&self) -> i32 {
        self.instance_id
    }
}

/// Struct of Relation to represent subscription relations.
#[derive(Default, Clone)]
pub struct RelationSet {
    pub(crate) bundle_name: String,
    pub(crate) expire_time: i64,
    // Database alias as key, subscription id generated by the cloud as value
    pub(crate) relations: HashMap<String, String>,
}

impl RelationSet {
    /// New a RelationSet.
    pub fn new(
        bundle_name: String,
        expire_time: i64,
        relations: HashMap<String, String>,
    ) -> RelationSet {
        RelationSet {
            bundle_name,
            expire_time,
            relations,
        }
    }

    /// Get the relating bundle name of this RelationSet instance.
    pub fn bundle_name(&self) -> &str {
        self.bundle_name.as_str()
    }

    /// Get expire time of this RelationSet instance.
    pub fn expire_time(&self) -> i64 {
        self.expire_time
    }

    /// Get the inner hashmap describing relations, with database alias as key, subscription id and
    /// time as value.
    pub fn relations(&self) -> &HashMap<String, String> {
        &self.relations
    }
}
