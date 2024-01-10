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

use crate::ipc_conn::connect::ValueBucket;
use crate::ipc_conn::error::Error;
use crate::ipc_conn::ffi::ConnectService;
use crate::ipc_conn::function::CloudDB::{
    Delete, GenerateIds, Heartbeat, Insert, Lock, Query, Unlock, Update,
};
use crate::ipc_conn::function::CloudServiceFunc::ConnectDatabase;
use crate::ipc_conn::*;
use ipc_rust::{
    BorrowedMsgParcel, Deserialize, IRemoteObj, IpcResult, MsgParcel, RemoteObj, String16,
};

pub(crate) type DatabaseStubResult<T> = Result<T, Error>;

#[derive(Debug)]
pub(crate) struct LockInfo {
    // Duration for which the cloud database is locked, in seconds.
    pub(crate) interval: i32,

    // Session ID for locking the cloud database.
    pub(crate) session_id: i32,
}

impl Deserialize for LockInfo {
    fn deserialize(parcel: &BorrowedMsgParcel<'_>) -> IpcResult<Self> {
        let interval = parcel.read::<i32>()?;
        let session_id = parcel.read::<i32>()?;
        let result = Self {
            interval,
            session_id,
        };
        Ok(result)
    }
}

pub(crate) struct DatabaseStub {
    pub(crate) remote_obj: Option<RemoteObj>,
}

impl DatabaseStub {
    pub(crate) fn new(
        user_id: i32,
        bundle_name: &str,
        database: &Database,
    ) -> DatabaseStubResult<Self> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let bundle_name = String16::new(bundle_name);
        msg_parcel
            .write(&bundle_name)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(database)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = ConnectDatabase as u32;
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

    pub(crate) fn generate_ids(&mut self, number: u32) -> DatabaseStubResult<Vec<String>> {
        #[derive(Default)]
        struct Ids(Vec<String>);

        impl Deserialize for Ids {
            fn deserialize(parcel: &BorrowedMsgParcel<'_>) -> ipc_rust::IpcResult<Self> {
                let id_vec = vec_raw_read::<String16>(parcel)?;
                let ids = Ids(id_vec.iter().map(|x| x.get_string()).collect());
                Ok(ids)
            }
        }

        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        msg_parcel
            .write(&number)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = GenerateIds as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let error = receive
            .read::<Error>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        if !error.eq(&Error::Success) {
            return Err(error);
        }
        let ids = receive
            .read::<Ids>()
            .map_err(|_| Error::ReadMsgParcelFailed)?
            .0;
        Ok(ids)
    }

    pub(crate) fn insert(
        &mut self,
        table: &str,
        values: &ValueBuckets,
        extends: &ValueBuckets,
    ) -> DatabaseStubResult<Vec<Option<ValueBucket>>> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let table = String16::new(table);
        msg_parcel
            .write(&table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(values)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(extends)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Insert as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let mut results = vec![];
        let length = receive
            .read::<i32>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        for _ in 0..length {
            let error = receive
                .read::<Error>()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            if !error.eq(&Error::Success) {
                results.push(None);
                continue;
            } else {
                let result = receive
                    .read::<ValueBucket>()
                    .map_err(|_| Error::ReadMsgParcelFailed)?;
                results.push(Some(result));
            }
        }
        Ok(results)
    }

    pub(crate) fn update(
        &mut self,
        table: &str,
        values: &ValueBuckets,
        extends: &ValueBuckets,
    ) -> DatabaseStubResult<Vec<Option<ValueBucket>>> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let table = String16::new(table);
        msg_parcel
            .write(&table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(values)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(extends)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Update as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let mut results = vec![];
        let length = receive
            .read::<i32>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        for _ in 0..length {
            let error = receive
                .read::<Error>()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            if !error.eq(&Error::Success) {
                results.push(None);
                continue;
            } else {
                let result = receive
                    .read::<ValueBucket>()
                    .map_err(|_| Error::ReadMsgParcelFailed)?;
                results.push(Some(result));
            }
        }
        Ok(results)
    }

    pub(crate) fn delete(
        &mut self,
        table: &str,
        extends: &ValueBuckets,
    ) -> DatabaseStubResult<Vec<Option<ValueBucket>>> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let table = String16::new(table);
        msg_parcel
            .write(&table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(extends)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Delete as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let mut results = vec![];
        let length = receive
            .read::<i32>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        for _ in 0..length {
            let error = receive
                .read::<Error>()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            if !error.eq(&Error::Success) {
                results.push(None);
                continue;
            } else {
                let result = receive
                    .read::<ValueBucket>()
                    .map_err(|_| Error::ReadMsgParcelFailed)?;
                results.push(Some(result));
            }
        }
        Ok(results)
    }

    pub(crate) fn query_values(
        &mut self,
        table: &str,
        columns: &[String],
        result_limit: i32,
        cursor: &str,
    ) -> DatabaseStubResult<CloudData> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        let table = String16::new(table);
        let columns: Vec<String16> = columns.iter().map(|x| String16::new(x)).collect();
        let cursor = String16::new(cursor);
        msg_parcel
            .write(&table)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(&columns)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(&result_limit)
            .map_err(|_| Error::WriteMsgParcelFailed)?;
        msg_parcel
            .write(&cursor)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Query as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let error = receive
            .read::<Error>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        if !error.eq(&Error::Success) {
            return Err(error);
        }
        let result = receive
            .read::<CloudData>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        Ok(result)
    }

    pub(crate) fn lock(&mut self, interval: i32) -> DatabaseStubResult<LockInfo> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        msg_parcel
            .write(&interval)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Lock as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let error = receive
            .read::<Error>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        if !error.eq(&Error::Success) {
            return Err(error);
        }
        let result = receive
            .read::<LockInfo>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        Ok(result)
    }

    pub(crate) fn unlock(&mut self, session_id: i32) -> DatabaseStubResult<()> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        msg_parcel
            .write(&session_id)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Unlock as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let error = receive
            .read::<Error>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        if !error.eq(&Error::Success) {
            return Err(error);
        }
        Ok(())
    }

    pub(crate) fn heartbeat(&self, session_id: i32) -> DatabaseStubResult<LockInfo> {
        let mut msg_parcel = MsgParcel::new().ok_or(Error::CreateMsgParcelFailed)?;
        msg_parcel
            .write(&session_id)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = Heartbeat as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let receive = remote_obj
            .send_request(function_number, &msg_parcel, false)
            .map_err(|_| Error::SendRequestFailed)?;

        let error = receive
            .read::<Error>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        if !error.eq(&Error::Success) {
            return Err(error);
        }
        let result = receive
            .read::<LockInfo>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        Ok(result)
    }
}

#[cfg(test)]
#[cfg(feature = "test_server_ready")]
mod database_server_ready {
    use crate::ipc_conn::database::DatabaseStub;
    use crate::ipc_conn::{Connect, FieldRaw, ValueBucket, ValueBuckets};

    /// UT test for database_stub_new.
    ///
    /// # Title
    /// ut_database_stub_new
    ///
    /// # Brief
    /// 1. Create a `DatabaseStub` struct.
    /// 2. Check if it is correct.
    #[test]
    fn ut_database_stub_new() {
        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut connect = Connect::new(user_id).unwrap();
        let app_schema = &connect.get_app_schema(user_id, &bundle_name).unwrap();
        let database = &app_schema.databases.0[0];

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let database = DatabaseStub::new(user_id, &bundle_name, database);
        assert!(database.is_ok());
    }

    /// UT test for database_stub_query.
    ///
    /// # Title
    /// ut_database_stub_query
    ///
    /// # Brief
    /// 1. Create a `DatabaseStub` struct.
    /// 2. Check if it is correct.
    #[test]
    fn ut_database_stub_query() {
        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut connect = Connect::new(user_id).unwrap();
        let app_schema = &connect.get_app_schema(user_id, &bundle_name).unwrap();
        let database = &app_schema.databases.0[0];

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut database = DatabaseStub::new(user_id, &bundle_name, database).unwrap();

        let table = "cloud_folders";
        let columns = [
            String::from("data"),
            String::from("recycled"),
            String::from("recycledTime"),
            String::from("uuid"),
        ];
        let result_limit = 3;
        let cursor = "";
        let result = database
            .query_values(table, &columns, result_limit, cursor)
            .unwrap();

        let table = "cloud_folders";
        let columns = [
            String::from("data"),
            String::from("recycled"),
            String::from("recycledTime"),
            String::from("uuid"),
        ];
        let result_limit = 3;
        let result = database.query_values(table, &columns, result_limit, &result.next_cursor);
        assert!(result.is_ok());

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut connect = Connect::new(user_id).unwrap();
        let app_schema = &connect.get_app_schema(user_id, &bundle_name).unwrap();
        let database = &app_schema.databases.0[0];

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut database = DatabaseStub::new(user_id, &bundle_name, database).unwrap();

        let table = "cloud_old_notes";
        let columns = [
            String::from("attachments"),
            String::from("data"),
            String::from("recycled"),
            String::from("recycledTime"),
            String::from("uuid"),
        ];
        let result_limit = 5;
        let cursor = "";
        let result = database
            .query_values(table, &columns, result_limit, cursor)
            .unwrap();

        let table = "cloud_old_notes";
        let columns = [
            String::from("attachments"),
            String::from("data"),
            String::from("recycled"),
            String::from("recycledTime"),
            String::from("uuid"),
        ];
        let result_limit = 5;
        let result = database.query_values(table, &columns, result_limit, &result.next_cursor);
        assert!(result.is_ok());
    }

    /// UT test for database_stub_insert.
    ///
    /// # Title
    /// ut_database_stub_insert
    ///
    /// # Brief
    /// 1. Create a `DatabaseStub` struct.
    /// 2. Check if it is correct.
    #[test]
    fn ut_database_stub_insert() {
        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut connect = Connect::new(user_id).unwrap();
        let app_schema = &connect.get_app_schema(user_id, &bundle_name).unwrap();
        let database = &app_schema.databases.0[0];

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut database = DatabaseStub::new(user_id, &bundle_name, database).unwrap();

        let table = "cloud_folders";
        let mut values = ValueBuckets::default();
        let mut data = ValueBucket::default();
        data.0.insert(
            String::from("data"),
            FieldRaw::Text(String::from("folder0")),
        );
        data.0
            .insert(String::from("recycled"), FieldRaw::Bool(true));
        data.0
            .insert(String::from("recycledTime"), FieldRaw::Number(0));
        data.0
            .insert(String::from("uuid"), FieldRaw::Text(String::from("uuid")));
        values.0.push(data);

        let mut extends = ValueBuckets::default();
        let mut data = ValueBucket::default();
        data.0
            .insert(String::from("id"), FieldRaw::Text(String::from("id")));
        data.0
            .insert(String::from("createTime"), FieldRaw::Number(1));
        data.0
            .insert(String::from("modifyTime"), FieldRaw::Number(2));
        data.0
            .insert(String::from("operation"), FieldRaw::Number(0));
        extends.0.push(data);
        let result = database.insert(table, &values, &extends);
        assert!(result.is_ok());

        let result = database.delete(table, &extends);
        assert!(result.is_ok());
    }

    /// UT test for database_stub_update.
    ///
    /// # Title
    /// ut_database_stub_update
    ///
    /// # Brief
    /// 1. Create a `DatabaseStub` struct.
    /// 2. Check if it is correct.
    #[test]
    fn ut_database_stub_update() {
        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut connect = Connect::new(user_id).unwrap();
        let app_schema = &connect.get_app_schema(user_id, &bundle_name).unwrap();
        let database = &app_schema.databases.0[0];

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut database = DatabaseStub::new(user_id, &bundle_name, database).unwrap();

        let table = "cloud_folders";
        let values = ValueBuckets::default();
        let extends = ValueBuckets::default();
        let result = database.update(table, &values, &extends);
        assert!(result.is_ok());
    }

    /// UT test for database_stub_delete.
    ///
    /// # Title
    /// ut_database_stub_delete
    ///
    /// # Brief
    /// 1. Create a `DatabaseStub` struct.
    /// 2. Check if it is correct.
    #[test]
    fn ut_database_stub_delete() {
        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut connect = Connect::new(user_id).unwrap();
        let app_schema = &connect.get_app_schema(user_id, &bundle_name).unwrap();
        let database = &app_schema.databases.0[0];

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut database = DatabaseStub::new(user_id, &bundle_name, database).unwrap();

        let table = "cloud_folders";
        let extends = ValueBuckets::default();
        let result = database.delete(table, &extends);
        assert!(result.is_ok());
    }

    /// UT test for ut_database_stub_lock_heart_beat_unlock.
    ///
    /// # Title
    /// ut_database_stub_lock_heart_beat_unlock
    ///
    /// # Brief
    /// 1. Create a `DatabaseStub` struct.
    /// 2. Check if it is correct.
    #[test]
    fn ut_database_stub_lock_heart_beat_unlock() {
        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut connect = Connect::new(user_id).unwrap();
        let app_schema = &connect.get_app_schema(user_id, &bundle_name).unwrap();
        let database = &app_schema.databases.0[0];

        let user_id = 100;
        let bundle_name = String::from("com.huawei.hmos.notepad");
        let mut database = DatabaseStub::new(user_id, &bundle_name, database).unwrap();
        let interval = 1000;
        let lock_info = database.lock(interval).unwrap();
        let result = database.heartbeat(lock_info.session_id);
        assert!(result.is_ok());

        let result = database.unlock(lock_info.session_id);
        assert!(result.is_ok());
    }
}
