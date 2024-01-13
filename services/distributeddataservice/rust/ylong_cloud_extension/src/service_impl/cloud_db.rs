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

use crate::service_impl::error::SyncError;
use crate::service_impl::types::{
    Database, RawValueBucket, Value, CREATE_FIELD, GID_FIELD, MODIFY_FIELD,
};
use crate::{ipc_conn, SyncResult};
use std::collections::HashMap;

const LOCK_EXPIRE: i32 = 60 * 60;

/// Struct of CloudDatabase
pub struct CloudDatabase {
    database_ipc: ipc_conn::DatabaseStub,
    database: Database,
    lock_session_id: Option<i32>,
}

impl CloudDatabase {
    /// Initialize a CloudDatabase with user id and database. This function will send IPC request
    /// with user id to get relating cloud database information.
    pub fn new(user_id: i32, bundle_name: &str, database: Database) -> SyncResult<CloudDatabase> {
        let db = ipc_conn::Database::try_from(&database)?;
        let database_ipc = ipc_conn::DatabaseStub::new(user_id, bundle_name, &db)?;
        Ok(CloudDatabase {
            database_ipc,
            database,
            lock_session_id: None,
        })
    }

    /// Execute a sql on the cloud database. Unsupported currently.
    pub fn execute(
        &mut self,
        _table: &str,
        _sql: &str,
        _extend: &RawValueBucket,
    ) -> SyncResult<()> {
        Err(SyncError::Unsupported)
    }

    /// Insert a batch of value buckets into the cloud database, with specific target table name.
    ///
    /// Values and extends are all value buckets from the users, but for the convenience, split
    /// them in parameters. Extends will also be changed to store information and results from
    /// the insertion action.
    pub fn batch_insert(
        &mut self,
        table: &str,
        values: &[RawValueBucket],
        extends: &mut Vec<RawValueBucket>,
    ) -> SyncResult<Vec<RawValueBucket>> {
        let ids = self.database_ipc.generate_ids(values.len() as u32)?;

        for (i, id) in ids.iter().enumerate() {
            if i < extends.len() {
                extends[i].insert(GID_FIELD.to_string(), Value::String(id.to_string()));
            } else {
                let mut vb = HashMap::new();
                vb.insert(GID_FIELD.to_string(), Value::String(id.to_string()));
                extends.push(vb);
            }
        }

        self.upload(
            table,
            values,
            extends,
            true,
            true,
            ipc_conn::DatabaseStub::insert,
        )
    }

    /// Update a batch of value buckets in the cloud database, with specific target table name.
    ///
    /// Values and extends are all value buckets from the users, but for the convenience, split
    /// them in parameters. Extends will also be changed to store information and results from
    /// the insertion action.
    pub fn batch_update(
        &mut self,
        table: &str,
        values: &[RawValueBucket],
        extends: &mut [RawValueBucket],
    ) -> SyncResult<Vec<RawValueBucket>> {
        self.upload(
            table,
            values,
            extends,
            false,
            false,
            ipc_conn::DatabaseStub::update,
        )
    }

    /// Delete a batch of value buckets from the cloud database, with specific target table name.
    ///
    /// Values and extends are all value buckets from the users, but for the convenience, split
    /// them in parameters. Extends will also be changed to store information and results from
    /// the insertion action.
    pub fn batch_delete(
        &mut self,
        table: &str,
        extends: &[RawValueBucket],
    ) -> SyncResult<Vec<RawValueBucket>> {
        match self.database.tables.get(table) {
            None => Err(SyncError::NoSuchTableInDb),
            Some(t) => {
                let name = &t.name;
                let mut records = vec![];
                for ext in extends.iter() {
                    match ext.get(GID_FIELD) {
                        Some(Value::String(gid)) => {
                            let mut extend_data = HashMap::new();
                            let modify_field = match ext.get(MODIFY_FIELD) {
                                Some(Value::Int(i)) => *i,
                                _ => 0,
                            };

                            extend_data
                                .insert("id".to_string(), ipc_conn::FieldRaw::Text(gid.clone()));
                            extend_data
                                .insert("operation".to_string(), ipc_conn::FieldRaw::Number(2_i64));
                            extend_data.insert(
                                "modifyTime".to_string(),
                                ipc_conn::FieldRaw::Number(modify_field),
                            );

                            records.push(ipc_conn::ValueBucket(extend_data));
                        }
                        _ => continue,
                    }
                }
                let extends = ipc_conn::ValueBuckets(records);
                let ret = self.database_ipc.delete(name, &extends)?;

                let mut results = vec![];
                for value_bucket in ret.into_iter().flatten() {
                    results.push(value_bucket.into());
                }
                Ok(results)
            }
        }
    }

    /// Query value buckets from the cloud database, with specific target table name. Return with
    /// the corresponding Cursor and ValueBuckets.
    ///
    /// Values and extends are all value buckets from the users, but for the convenience, split
    /// them in parameters. Extends will also be changed to store information and results from
    /// the insertion action.
    pub fn batch_query(&mut self, table: &str, cursor: &str) -> SyncResult<CloudDbData> {
        const QUERY_LIMIT: usize = 10;

        match self.database.tables.get(table) {
            None => Err(SyncError::NoSuchTableInDb),
            Some(t) => {
                let name = &t.name;
                let mut fields = vec![];
                for field in &t.fields {
                    fields.push(field.col_name.clone());
                }
                let ret =
                    self.database_ipc
                        .query_values(name, &fields, QUERY_LIMIT as i32, cursor)?;
                Ok(ret.into())
            }
        }
    }

    /// Send lock request to the other end through IPC message, so that users can get exclusive
    /// access to the database.
    ///
    /// Return err if that fails.
    pub fn lock(&mut self) -> SyncResult<i32> {
        if self.lock_session_id.is_some() {
            return Ok(0);
        }
        let int = self.database_ipc.lock(LOCK_EXPIRE)?;
        self.lock_session_id = Some(int.session_id);
        Ok(int.interval)
    }

    /// Send unlock request to the other end through IPC message, so that users can release exclusive
    /// access to the database, and others can use it.
    ///
    /// Return err if that fails.
    pub fn unlock(&mut self) -> SyncResult<()> {
        match self.lock_session_id {
            Some(i) => {
                self.database_ipc.unlock(i)?;
                self.lock_session_id = None;
                Ok(())
            }
            None => Err(SyncError::SessionUnlocked),
        }
    }

    /// Send heartbeat to the other end through IPC message, so that the users can prolong exclusive
    /// access to the database. If no action is taken, this access will expire after some time (by
    /// default, 60 min).
    pub fn heartbeat(&mut self) -> SyncResult<()> {
        match self.lock_session_id {
            Some(i) => {
                self.database_ipc.heartbeat(i)?;
                Ok(())
            }
            None => Err(SyncError::SessionUnlocked),
        }
    }

    fn upload<F>(
        &mut self,
        table: &str,
        values: &[RawValueBucket],
        extends: &mut [RawValueBucket],
        is_new: bool,
        is_backfill: bool,
        mut f: F,
    ) -> SyncResult<Vec<RawValueBucket>>
    where
        F: FnMut(
            &mut ipc_conn::DatabaseStub,
            &str,
            &ipc_conn::ValueBuckets,
            &ipc_conn::ValueBuckets,
        ) -> Result<Vec<Option<ipc_conn::ValueBucket>>, ipc_conn::Error>,
    {
        let mut asset_keys = vec![];
        let mut extend_ids = vec![];

        match self.database.tables.get(table) {
            None => Err(SyncError::NoSuchTableInDb),
            Some(t) => {
                let name = &t.name;
                let mut values_records = vec![];
                let mut extends_records = vec![];
                for (bucket, ext) in values.iter().zip(extends.iter_mut()) {
                    match ext.get(GID_FIELD) {
                        Some(Value::String(gid)) => {
                            let mut extend_data = HashMap::new();
                            let create_field = match ext.get(CREATE_FIELD) {
                                Some(Value::Int(i)) => *i,
                                _ => 0,
                            };

                            let modify_field = match ext.get(MODIFY_FIELD) {
                                Some(Value::Int(i)) => *i,
                                _ => 0,
                            };

                            extend_ids.push(gid.clone());
                            extend_data
                                .insert("id".to_string(), ipc_conn::FieldRaw::Text(gid.clone()));
                            if is_new {
                                extend_data.insert(
                                    "operation".to_string(),
                                    ipc_conn::FieldRaw::Number(0_i64),
                                );
                            } else {
                                extend_data.insert(
                                    "operation".to_string(),
                                    ipc_conn::FieldRaw::Number(1_i64),
                                );
                            }
                            extend_data.insert(
                                "createTime".to_string(),
                                ipc_conn::FieldRaw::Number(create_field),
                            );
                            extend_data.insert(
                                "modifyTime".to_string(),
                                ipc_conn::FieldRaw::Number(modify_field),
                            );

                            let mut record_data = HashMap::new();
                            for (key, value) in bucket {
                                if value.has_asset() {
                                    asset_keys.push(key.to_string());
                                    ext.insert(key.to_string(), value.clone());
                                }
                                let field = ipc_conn::FieldRaw::from(value);
                                record_data.insert(key.clone(), field);
                            }

                            values_records.push(ipc_conn::ValueBucket(record_data));
                            extends_records.push(ipc_conn::ValueBucket(extend_data));
                        }
                        _ => continue,
                    }
                }
                let value_raw_vb = ipc_conn::ValueBuckets(values_records);
                let extend_raw_vb = ipc_conn::ValueBuckets(extends_records);

                let mut results: Vec<RawValueBucket> = vec![];
                let ret = f(&mut self.database_ipc, name, &value_raw_vb, &extend_raw_vb)?;
                for value_bucket in ret.into_iter().flatten() {
                    results.push(value_bucket.into());
                }

                if is_backfill {
                    let mut result_ids = vec![];
                    for result in &results {
                        if let Some(Value::String(id)) = result.get("id") {
                            result_ids.push(id.to_string());
                        }
                    }

                    for (result_idx, result_id) in result_ids.iter().enumerate() {
                        for (ext_idx, ext_id) in extend_ids.iter().enumerate() {
                            if result_id == ext_id {
                                for asset_key in &asset_keys {
                                    *extends[ext_idx].get_mut(asset_key).unwrap() =
                                        results[result_idx].get(asset_key).unwrap().clone();
                                }
                            }
                        }
                    }
                }

                Ok(results)
            }
        }
    }
}

/// Struct of CloudDbData.
#[derive(Clone, Debug)]
pub struct CloudDbData {
    pub(crate) next_cursor: String,
    pub(crate) has_more: bool,
    pub(crate) values: Vec<ipc_conn::ValueBucket>,
}

impl From<ipc_conn::CloudData> for CloudDbData {
    fn from(value: ipc_conn::CloudData) -> Self {
        let mut vec = vec![];
        for v in value.values.0 {
            vec.push(v);
        }
        CloudDbData {
            next_cursor: value.next_cursor,
            has_more: value.has_more,
            values: vec,
        }
    }
}

impl CloudDbData {
    /// Get next cursor from CloudDbData instance.
    pub fn next_cursor(&self) -> &str {
        &self.next_cursor
    }

    /// Check whether CloudDbData instance has more value buckets.
    pub fn has_more(&self) -> bool {
        self.has_more
    }

    /// Get values from CloudDbData instance.
    pub fn values(&self) -> &[ipc_conn::ValueBucket] {
        &self.values
    }
}
