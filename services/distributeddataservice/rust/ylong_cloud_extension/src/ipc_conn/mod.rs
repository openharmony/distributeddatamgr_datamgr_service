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

mod asset;
mod connect;
mod database;
mod error;
mod ffi;
mod function;

use std::collections::HashMap;

pub(crate) use asset::AssetLoader;
pub use asset::{CloudAsset, CloudAssets};
pub(crate) use connect::{
    AppInfo, CloudData, Connect, Database, Databases, Field, FieldRaw, FieldType, Fields,
    OrderTable, Schema, SchemaOrderTables, ServiceInfo, UnsubscriptionInfo, ValueBuckets,
};
pub use connect::{AssetStatus, ValueBucket};
pub(crate) use database::DatabaseStub;
/// Error message at the ipc.
pub use error::Error;
pub(crate) use error::Errors;
use ipc::parcel::{Deserialize, MsgParcel, Serialize};
use ipc::IpcResult;

pub(crate) fn string_hash_map_raw_write<V: Serialize>(
    msg_parcel: &mut MsgParcel,
    hash_map: &HashMap<String, V>,
) -> IpcResult<()> {
    msg_parcel.write(&(hash_map.len() as i32))?;
    let mut keys: Vec<&String> = hash_map.keys().collect();
    keys.sort();
    for key in keys {
        msg_parcel.write_string16(&String::from(key))?;
        msg_parcel.write(&hash_map[key])?;
    }
    Ok(())
}

pub(crate) fn string_hash_map_raw_read<V: Deserialize>(
    msg_parcel: &mut MsgParcel,
) -> IpcResult<HashMap<String, V>> {
    let length = msg_parcel.read::<i32>()?;
    let mut hash_map = HashMap::new();
    for _ in 0..length {
        let key = msg_parcel.read_string16()?;
        let value = msg_parcel.read::<V>()?;
        hash_map.insert(key, value);
    }
    Ok(hash_map)
}

pub(crate) fn vec_raw_write<T: Serialize>(
    msg_parcel: &mut MsgParcel,
    vector: &Vec<T>,
) -> IpcResult<()> {
    msg_parcel.write(&(vector.len() as i32))?;
    for value in vector {
        msg_parcel.write(value)?;
    }
    Ok(())
}

pub(crate) fn vec_raw_read<T: Deserialize>(msg_parcel: &mut MsgParcel) -> IpcResult<Vec<T>> {
    let length = msg_parcel.read::<i32>()? as usize;
    let mut vector = Vec::with_capacity(length);
    for _ in 0..length {
        let value = msg_parcel.read::<T>()?;
        vector.push(value);
    }
    Ok(vector)
}

#[cfg(test)]
mod test {
    use ipc::parcel::MsgParcel;

    use crate::ipc_conn::{vec_raw_read, vec_raw_write};

    /// UT test for vec_raw_write.
    ///
    /// # Title
    /// ut_vec_raw_write
    ///
    /// # Brief
    /// 1. Create a `Vec` and `MsgParcel` struct.
    /// 2. Write the data in the `Vec` to the `MsgParcel`.
    /// 3. Read the data and check if it is correct.
    #[test]
    fn ut_vec_raw_write() {
        let vector = vec![
            String::from("value1"),
            String::from("value2"),
            String::from("value3"),
        ];

        let mut parcel = MsgParcel::new();
        assert!(vec_raw_write(&mut parcel, &vector).is_ok());

        assert_eq!(parcel.read::<i32>().unwrap(), 3);
        assert_eq!(parcel.read::<String>().unwrap(), String::from("value1"));
        assert_eq!(parcel.read::<String>().unwrap(), String::from("value2"));
        assert_eq!(parcel.read::<String>().unwrap(), String::from("value3"));
    }

    /// UT test for vec_raw_read.
    ///
    /// # Title
    /// ut_vec_raw_read
    ///
    /// # Brief
    /// 1. Create a `Vec` and `MsgParcel` struct.
    /// 2. Write the data in the `Vec` to the `MsgParcel`.
    /// 3. Read the data in the `MsgParcel` to the `Vec`.
    /// 4. Check if it is correct.
    #[test]
    fn ut_vec_raw_read() {
        let vector = vec![
            String::from("value1"),
            String::from("value2"),
            String::from("value3"),
        ];

        let mut parcel = MsgParcel::new();
        assert!(vec_raw_write(&mut parcel, &vector).is_ok());

        let result = vec_raw_read::<String>(&mut parcel).unwrap();
        assert_eq!(vector, result);
    }
}
