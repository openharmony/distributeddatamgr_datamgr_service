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

use std::collections::HashMap;
use std::sync::{RwLock, RwLockReadGuard};

use ipc::parcel::{Deserialize, MsgParcel, Serialize};
use ipc::remote::RemoteObj;
use ipc::{IpcResult, IpcStatusCode};

use crate::ipc_conn::asset::{CloudAsset, CloudAssets};
use crate::ipc_conn::error::{Error, Errors};
use crate::ipc_conn::ffi::ConnectService;
use crate::ipc_conn::function::CloudServiceFunc::{
    GetAppBriefInfo, GetAppSchema, GetServiceInfo, Subscribe, Unsubscribe,
};
use crate::ipc_conn::{
    string_hash_map_raw_read, string_hash_map_raw_write, vec_raw_read, vec_raw_write,
};

/// ************ All AppSchema struct-related structures and methods
/// *************

#[derive(Default, Debug, PartialEq, Clone)]
pub(crate) struct Response {
    pub(crate) time: u64,
    pub(crate) device_name: String,
    pub(crate) app_id: String,
}

impl Serialize for Response {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        parcel.write(&self.time)?;
        parcel.write_string16(&self.device_name)?;
        parcel.write_string16(&self.app_id)?;
        Ok(())
    }
}

impl Deserialize for Response {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let time = parcel.read::<u64>()?;
        let device_name = parcel.read_string16()?;
        let app_id = parcel.read_string16()?;

        let result = Response {
            time,
            device_name,
            app_id,
        };
        Ok(result)
    }
}

/// Struct that mark status of an Asset.
#[repr(C)]
#[derive(Eq, Clone, Debug, PartialEq)]
pub enum AssetStatus {
    /// Normal. Everything works fine for the asset.
    Normal,
    /// The asset is about to be inserted.
    Insert,
    /// The asset is about to be updated.
    Update,
    /// The asset is about to be deleted.
    Delete,
    /// The asset is abnormal and something is wrong with it.
    Abnormal,
    /// The asset is being downloading.
    Downloading,
}

impl Default for AssetStatus {
    fn default() -> Self {
        Self::Normal
    }
}

impl Serialize for AssetStatus {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        match self {
            AssetStatus::Normal => parcel.write(&0_i32),
            AssetStatus::Insert => parcel.write(&1_i32),
            AssetStatus::Update => parcel.write(&2_i32),
            AssetStatus::Delete => parcel.write(&3_i32),
            AssetStatus::Abnormal => parcel.write(&4_i32),
            AssetStatus::Downloading => parcel.write(&5_i32),
        }
    }
}

impl Deserialize for AssetStatus {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let index = parcel.read::<i32>()?;
        match index {
            0 => Ok(AssetStatus::Normal),
            1 => Ok(AssetStatus::Insert),
            2 => Ok(AssetStatus::Update),
            3 => Ok(AssetStatus::Delete),
            4 => Ok(AssetStatus::Abnormal),
            5 => Ok(AssetStatus::Downloading),
            _ => Err(IpcStatusCode::Failed),
        }
    }
}

#[derive(Debug, PartialEq)]
pub(crate) enum SwitchStatus {
    Close,
    Open,
    NotEnable,
}

impl Default for SwitchStatus {
    fn default() -> Self {
        Self::Close
    }
}

impl Serialize for SwitchStatus {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        match self {
            SwitchStatus::Close => parcel.write(&0_u32),
            SwitchStatus::Open => parcel.write(&1_u32),
            SwitchStatus::NotEnable => parcel.write(&2_u32),
        }
    }
}

impl Deserialize for SwitchStatus {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let index = parcel.read::<u32>()?;
        match index {
            0 => Ok(SwitchStatus::Close),
            1 => Ok(SwitchStatus::Open),
            2 => Ok(SwitchStatus::NotEnable),
            _ => Err(IpcStatusCode::Failed),
        }
    }
}

#[derive(Debug, Clone)]
pub enum FieldRaw {
    Null,
    Number(i64),
    Real(f64),
    Text(String),
    Bool(bool),
    Blob(Vec<u8>),
    Asset(CloudAsset),
    Assets(CloudAssets),
}

impl From<FieldRaw> for u8 {
    fn from(value: FieldRaw) -> Self {
        match value {
            FieldRaw::Null => 0,
            FieldRaw::Number(_) => 1,
            FieldRaw::Real(_) => 2,
            FieldRaw::Text(_) => 3,
            FieldRaw::Bool(_) => 4,
            FieldRaw::Blob(_) => 5,
            FieldRaw::Asset(_) => 6,
            FieldRaw::Assets(_) => 7,
        }
    }
}

impl Serialize for FieldRaw {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        match self {
            FieldRaw::Null => parcel.write(&0_u32),
            FieldRaw::Number(number) => {
                parcel.write(&1_u32)?;
                parcel.write(number)
            }
            FieldRaw::Real(real) => {
                parcel.write(&2_u32)?;
                parcel.write(real)
            }
            FieldRaw::Text(text) => {
                parcel.write(&3_u32)?;
                parcel.write_string16(text)
            }
            FieldRaw::Bool(boolean) => {
                parcel.write(&4_u32)?;
                parcel.write(boolean)
            }
            FieldRaw::Blob(blob) => {
                parcel.write(&5_u32)?;
                parcel.write(blob)
            }
            FieldRaw::Asset(asset) => {
                parcel.write(&6_u32)?;
                parcel.write(asset)
            }
            FieldRaw::Assets(assets) => {
                parcel.write(&7_u32)?;
                parcel.write(assets)
            }
        }
    }
}

impl Deserialize for FieldRaw {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let index = parcel.read::<u32>()?;
        match index {
            0 => Ok(FieldRaw::Null),
            1 => {
                let number = parcel.read::<i64>()?;
                Ok(FieldRaw::Number(number))
            }
            2 => {
                let real = parcel.read::<f64>()?;
                Ok(FieldRaw::Real(real))
            }
            3 => {
                let text = parcel.read_string16()?;
                Ok(FieldRaw::Text(text))
            }
            4 => {
                let boolean = parcel.read::<bool>()?;
                Ok(FieldRaw::Bool(boolean))
            }
            5 => {
                let blob = parcel.read::<Vec<u8>>()?;
                Ok(FieldRaw::Blob(blob))
            }
            6 => {
                let asset = parcel.read::<CloudAsset>()?;
                Ok(FieldRaw::Asset(asset))
            }
            7 => {
                let assets = parcel.read::<CloudAssets>()?;
                Ok(FieldRaw::Assets(assets))
            }
            _ => Err(IpcStatusCode::Failed),
        }
    }
}

/// Struct ValueBucket.
#[derive(Default, Debug, Clone)]
pub struct ValueBucket(pub HashMap<String, FieldRaw>);

impl Serialize for ValueBucket {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        string_hash_map_raw_write(parcel, &self.0)
    }
}

impl Deserialize for ValueBucket {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = string_hash_map_raw_read::<FieldRaw>(parcel)?;
        Ok(ValueBucket(result))
    }
}

#[derive(Default, Debug)]
pub(crate) struct ValueBuckets(pub(crate) Vec<ValueBucket>);

impl Serialize for ValueBuckets {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        vec_raw_write(parcel, &self.0)
    }
}

impl Deserialize for ValueBuckets {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = ValueBuckets(vec_raw_read::<ValueBucket>(parcel)?);
        Ok(result)
    }
}

#[derive(Default, Debug)]
pub(crate) struct CloudData {
    pub(crate) next_cursor: String,
    pub(crate) has_more: bool,
    pub(crate) values: ValueBuckets,
}

impl Deserialize for CloudData {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let next_cursor = parcel.read_string16()?;
        let has_more = parcel.read::<bool>()?;
        let values = parcel.read::<ValueBuckets>()?;

        let cloud_data = CloudData {
            next_cursor,
            has_more,
            values,
        };
        Ok(cloud_data)
    }
}

#[derive(Default)]
pub(crate) struct Database {
    pub(crate) alias: String,
    pub(crate) name: String,
    pub(crate) tables: SchemaOrderTables,
}

impl Serialize for Database {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        parcel.write_string16(&self.alias)?;
        parcel.write_string16(&self.name)?;
        parcel.write(&self.tables)?;
        Ok(())
    }
}

impl Deserialize for Database {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let alias = parcel.read_string16()?;
        let name = parcel.read_string16()?;
        let tables = parcel.read::<SchemaOrderTables>()?;

        let result = Database {
            name,
            alias,
            tables,
        };
        Ok(result)
    }
}

#[derive(Default, Debug, PartialEq)]
pub(crate) enum FieldType {
    #[default]
    Null,
    Number,
    Real,
    Text,
    Bool,
    Blob,
    Asset,
    Assets,
}

impl From<&FieldType> for u8 {
    fn from(value: &FieldType) -> Self {
        match value {
            FieldType::Null => 0,
            FieldType::Number => 1,
            FieldType::Real => 2,
            FieldType::Text => 3,
            FieldType::Bool => 4,
            FieldType::Blob => 5,
            FieldType::Asset => 6,
            FieldType::Assets => 7,
        }
    }
}

impl TryFrom<u8> for FieldType {
    type Error = Error;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        let typ = match value {
            0 => FieldType::Null,
            1 => FieldType::Number,
            2 => FieldType::Real,
            3 => FieldType::Text,
            4 => FieldType::Bool,
            5 => FieldType::Blob,
            6 => FieldType::Asset,
            7 => FieldType::Assets,
            _ => return Err(Error::InvalidFieldType),
        };
        Ok(typ)
    }
}

impl Serialize for FieldType {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        let code = u8::from(self) as u32;
        parcel.write(&code)
    }
}

impl Deserialize for FieldType {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let index = parcel.read::<i32>()?;
        match index {
            0 => Ok(FieldType::Null),
            1 => Ok(FieldType::Number),
            2 => Ok(FieldType::Real),
            3 => Ok(FieldType::Text),
            4 => Ok(FieldType::Bool),
            5 => Ok(FieldType::Blob),
            6 => Ok(FieldType::Asset),
            7 => Ok(FieldType::Assets),
            _ => Err(IpcStatusCode::Failed),
        }
    }
}

#[derive(Default, Debug, PartialEq)]
pub(crate) struct Field {
    pub(crate) col_name: String,
    pub(crate) alias: String,
    pub(crate) typ: FieldType,
    pub(crate) primary: bool,
    pub(crate) nullable: bool,
}

impl Serialize for Field {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        parcel.write_string16(&self.alias)?;
        parcel.write_string16(&self.col_name)?;
        parcel.write(&self.typ)?;
        parcel.write(&self.primary)?;
        parcel.write(&self.nullable)?;
        Ok(())
    }
}

impl Deserialize for Field {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let col_name = parcel.read_string16()?;
        let cloud_name = parcel.read_string16()?;
        let typ = parcel.read::<FieldType>()?;
        let primary = parcel.read::<bool>()?;
        let nullable = parcel.read::<bool>()?;

        let result = Field {
            col_name,
            alias: cloud_name,
            typ,
            primary,
            nullable,
        };
        Ok(result)
    }
}

#[derive(Default, Debug, PartialEq)]
pub(crate) struct Fields(pub(crate) Vec<Field>);

impl Serialize for Fields {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        vec_raw_write(parcel, &self.0)
    }
}

impl Deserialize for Fields {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = Fields(vec_raw_read::<Field>(parcel)?);
        Ok(result)
    }
}

#[derive(Default, Debug, PartialEq)]
pub(crate) struct OrderTable {
    pub(crate) alias: String,
    pub(crate) table_name: String,
    pub(crate) fields: Fields,
}

impl Serialize for OrderTable {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        parcel.write_string16(&self.alias)?;
        parcel.write_string16(&self.table_name)?;
        parcel.write(&self.fields)?;
        Ok(())
    }
}

impl Deserialize for OrderTable {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = OrderTable {
            alias: parcel.read_string16()?,
            table_name: parcel.read_string16()?,
            fields: parcel.read::<Fields>()?,
        };
        Ok(result)
    }
}

#[derive(Default, Debug, PartialEq)]
pub(crate) struct SchemaOrderTables(pub(crate) Vec<OrderTable>);

impl Serialize for SchemaOrderTables {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        vec_raw_write(parcel, &self.0)
    }
}

impl Deserialize for SchemaOrderTables {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = SchemaOrderTables(vec_raw_read::<OrderTable>(parcel)?);
        Ok(result)
    }
}

#[derive(Default)]
pub(crate) struct Databases(pub(crate) Vec<Database>);

impl Serialize for Databases {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        vec_raw_write(parcel, &self.0)
    }
}

impl Deserialize for Databases {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let databases = Databases(vec_raw_read(parcel)?);
        Ok(databases)
    }
}

#[derive(Default)]
pub(crate) struct Schema {
    pub(crate) version: i32,
    pub(crate) bundle_name: String,
    pub(crate) databases: Databases,
}

impl Schema {
    fn read(&mut self, msg_parcel: &mut MsgParcel) -> Result<(), Error> {
        if msg_parcel
            .read::<i32>()
            .map_err(|_| Error::ReadMsgParcelFailed)?
            == 0
        {
            self.version = msg_parcel
                .read::<i32>()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            self.bundle_name = msg_parcel
                .read_string16()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            self.databases = msg_parcel
                .read::<Databases>()
                .map_err(|_| Error::ReadMsgParcelFailed)?;
            Ok(())
        } else {
            Err(Error::ReadMsgParcelFailed)
        }
    }
}

impl Deserialize for Schema {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let version = parcel.read::<i32>()?;
        let bundle_name = parcel.read_string16()?;
        let databases = parcel.read::<Databases>()?;

        let result = Schema {
            version,
            bundle_name,
            databases,
        };
        Ok(result)
    }
}

/// ************ All AppInfo struct-related structures and methods *************

#[derive(Default, Debug, PartialEq)]
pub(crate) struct AppInfo {
    pub(crate) app_id: String,
    pub(crate) bundle_name: String,
    pub(crate) cloud_switch: bool,
    pub(crate) instance_id: i32,
}

impl Deserialize for AppInfo {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let app_id = parcel.read_string16()?;
        let bundle_name = parcel.read_string16()?;
        let cloud_switch = parcel.read::<i32>()? != 0;
        let instance_id = parcel.read::<i32>()?;

        let result = Self {
            app_id,
            bundle_name,
            cloud_switch,
            instance_id,
        };
        Ok(result)
    }
}

/// ************ All ServiceInfo struct-related structures and methods
/// *************
#[derive(Default)]
pub(crate) struct ServiceInfo {
    // Whether cloud is enabled.
    pub(crate) enable_cloud: bool,

    // ID of the cloud account generated by using SHA-256.
    pub(crate) account_id: String,

    // Total space (in KB) of the account on the server.
    pub(crate) total_space: u64,

    // Available space (in KB) of the account on the server.
    pub(crate) remain_space: u64,

    // Current user of the device.
    pub(crate) user: i32,
}

impl ServiceInfo {
    fn read(&mut self, msg_parcel: &mut MsgParcel) -> Result<(), Error> {
        self.enable_cloud = msg_parcel
            .read::<bool>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        self.account_id = msg_parcel
            .read_string16()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        self.total_space = msg_parcel
            .read::<u64>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        self.remain_space = msg_parcel
            .read::<u64>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        self.user = msg_parcel
            .read::<i32>()
            .map_err(|_| Error::ReadMsgParcelFailed)?;
        Ok(())
    }
}

/// ************ All Subscribe struct-related structures and methods
/// *************

#[derive(Default)]
#[non_exhaustive]
pub(crate) struct Subscription;

#[derive(Default, Debug, PartialEq)]
pub(crate) struct SubscriptionResultValue(pub(crate) Vec<(String, String)>);

impl Deserialize for SubscriptionResultValue {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let length = parcel.read::<i32>()? as usize;
        let mut vector = Vec::with_capacity(length);
        for _ in 0..length {
            let alias = parcel.read_string16()?;
            let subscription_id = parcel.read_string16()?;
            vector.push((alias, subscription_id));
        }
        Ok(SubscriptionResultValue(vector))
    }
}

#[derive(Default, Debug)]
pub(crate) struct SubscriptionResult(pub(crate) (i64, HashMap<String, SubscriptionResultValue>));

impl Deserialize for SubscriptionResult {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let interval = parcel.read::<i64>()?;
        let result = string_hash_map_raw_read::<SubscriptionResultValue>(parcel)?;
        Ok(SubscriptionResult((interval, result)))
    }
}

#[derive(Default)]
pub(crate) struct UnsubscriptionInfo(pub(crate) HashMap<String, Vec<String>>);

impl Serialize for UnsubscriptionInfo {
    fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        string_hash_map_raw_write(parcel, &self.0)
    }
}

impl Subscription {
    // For subsequent extensibility, use `&self` here.
    pub(crate) fn subscribe(
        &self,
        remote: &Option<RemoteObj>,
        expiration: i64,
        bundle_name: &str,
        databases: &Databases,
    ) -> Result<SubscriptionResult, Errors> {
        let mut msg_parcel = MsgParcel::new();

        msg_parcel
            .write(&expiration)
            .map_err(|_| Errors(vec![Error::WriteMsgParcelFailed]))?;
        msg_parcel
            .write_string16(bundle_name)
            .map_err(|_| Errors(vec![Error::WriteMsgParcelFailed]))?;
        msg_parcel
            .write(databases)
            .map_err(|_| Errors(vec![Error::WriteMsgParcelFailed]))?;

        let function_number = Subscribe as u32;
        let remote_obj = remote
            .as_ref()
            .ok_or(Errors(vec![Error::CreateMsgParcelFailed]))?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Errors(vec![Error::SendRequestFailed]))?;

        let mut errs = Errors::default();
        errs.0.push(Error::ReadMsgParcelFailed);
        if receive.read::<i32>().map_err(|_| errs)? == 0 {
            if let Ok(cloud_subscription_info) = receive.read::<SubscriptionResult>() {
                return Ok(cloud_subscription_info);
            }
        }

        let mut errs = Errors::default();
        errs.0.push(Error::ReadMsgParcelFailed);
        Err(errs)
    }

    pub(crate) fn unsubscribe(
        &self,
        remote: &Option<RemoteObj>,
        info: &UnsubscriptionInfo,
    ) -> Result<(), Errors> {
        let mut msg_parcel = MsgParcel::new();

        msg_parcel
            .write(info)
            .map_err(|_| Errors(vec![Error::WriteMsgParcelFailed]))?;

        let function_number = Unsubscribe as u32;
        let remote_obj = remote
            .as_ref()
            .ok_or(Errors(vec![Error::CreateMsgParcelFailed]))?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Errors(vec![Error::SendRequestFailed]))?;

        if let Ok(errs) = receive.read::<Errors>() {
            return Err(errs);
        }

        Ok(())
    }
}

/// ************ All Connect struct-related structures and methods *************

pub(crate) type ConnectResult<T> = Result<T, Error>;

pub(crate) struct Connect {
    pub(crate) remote_obj: Option<RemoteObj>,
    infos: HashMap<i32, Infos>,
}

impl Connect {
    // Creates a structure `Connect` for connecting to the cloud and getting various
    // information about the cloud.
    pub(crate) fn new(user_id: i32) -> ConnectResult<Self> {
        let remote_obj = unsafe { RemoteObj::from_ciremote(ConnectService(user_id)) }
            .ok_or(Error::GetProxyObjectFailed)?;
        Ok(Self {
            remote_obj: Some(remote_obj),
            infos: HashMap::default(),
        })
    }

    // Sends `MsgParcel` to the cloud,
    // wait for the return value from the cloud,
    // get the required app infos from the cloud.
    pub(crate) fn get_app_brief_info(
        &mut self,
        user_id: i32,
    ) -> ConnectResult<RwLockReadGuard<AppInfos>> {
        self.infos.entry(user_id).or_default();
        let infos = self.infos.get_mut(&user_id).unwrap();

        let mut lock = infos.app_infos.write().unwrap();

        let mut msg_parcel = MsgParcel::new();

        let function_number = GetAppBriefInfo as u32;
        let remote_obj = self
            .remote_obj
            .as_ref()
            .ok_or(Error::GetProxyObjectFailed)?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Error::SendRequestFailed)?;

        lock.0 = receive
            .read::<AppInfos>()
            .map_err(|_| Error::ReadMsgParcelFailed)?
            .0;

        drop(lock);

        Ok(infos.app_infos.read().unwrap())
    }

    // Sends `MsgParcel` to the cloud,
    // wait for the return value from the cloud,
    // get the required user info from the cloud.
    pub(crate) fn get_service_info(
        &mut self,
        user_id: i32,
    ) -> ConnectResult<RwLockReadGuard<ServiceInfo>> {
        self.infos.entry(user_id).or_default();
        let infos = self.infos.get_mut(&user_id).unwrap();

        let mut lock = infos.service_info.write().unwrap();

        let mut msg_parcel = MsgParcel::new();

        let function_number = GetServiceInfo as u32;
        let remote_obj = self
            .remote_obj
            .clone()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Error::SendRequestFailed)?;

        lock.read(&mut receive)?;

        drop(lock);

        Ok(infos.service_info.read().unwrap())
    }

    pub(crate) fn get_app_schema(
        &mut self,
        user_id: i32,
        bundle_name: &str,
    ) -> ConnectResult<RwLockReadGuard<Schema>> {
        self.infos.entry(user_id).or_default();
        let infos = self.infos.get_mut(&user_id).unwrap();

        let mut lock = infos.app_schema.write().unwrap();

        let mut msg_parcel = MsgParcel::new();

        msg_parcel
            .write_string16(bundle_name)
            .map_err(|_| Error::WriteMsgParcelFailed)?;

        let function_number = GetAppSchema as u32;
        let remote_obj = self
            .remote_obj
            .clone()
            .ok_or(Error::CreateMsgParcelFailed)?;
        let mut receive = remote_obj
            .send_request(function_number, &mut msg_parcel)
            .map_err(|_| Error::SendRequestFailed)?;

        lock.read(&mut receive)?;

        drop(lock);

        Ok(infos.app_schema.read().unwrap())
    }

    pub(crate) fn subscribe(
        &self,
        expiration: i64,
        bundle_name: &str,
        databases: &Databases,
    ) -> Result<SubscriptionResult, Errors> {
        let subscription = Subscription::default();
        subscription.subscribe(&self.remote_obj, expiration, bundle_name, databases)
    }

    pub(crate) fn unsubscribe(&self, info: &UnsubscriptionInfo) -> Result<(), Errors> {
        let subscription = Subscription::default();
        subscription.unsubscribe(&self.remote_obj, info)
    }
}

#[derive(Default)]
pub(crate) struct AppInfos(pub(crate) HashMap<String, AppInfo>);

impl Deserialize for AppInfos {
    fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let result = AppInfos(string_hash_map_raw_read::<AppInfo>(parcel)?);
        Ok(result)
    }
}

#[derive(Default)]
pub(crate) struct Infos {
    app_infos: RwLock<AppInfos>,
    service_info: RwLock<ServiceInfo>,
    app_schema: RwLock<Schema>,
}

#[cfg(test)]
mod connect_test {
    use ipc::parcel::MsgParcel;

    use crate::ipc_conn::connect::{Response, SubscriptionResultValue, SwitchStatus};
    use crate::ipc_conn::{
        AppInfo, AssetStatus, CloudAsset, CloudAssets, CloudData, Database, Databases, FieldRaw,
        OrderTable, Schema, SchemaOrderTables, ValueBucket, ValueBuckets,
    };

    /// UT test for response_serialize.
    ///
    /// # Title
    /// ut_response_serialize
    ///
    /// # Brief
    /// 1. Create a `Response` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_response_serialize() {
        let response = Response::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&response).unwrap();

        assert_eq!(msg_parcel.read::<i64>().unwrap(), 0);
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
    }

    /// UT test for response_deserialize.
    ///
    /// # Title
    /// ut_response_deserialize
    ///
    /// # Brief
    /// 1. Create a `Response` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `Response`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_response_deserialize() {
        let response = Response::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&response).unwrap();

        assert_eq!(msg_parcel.read::<Response>().unwrap(), response);
    }

    /// UT test for operation_type_serialize.
    ///
    /// # Title
    /// ut_operation_type_serialize
    ///
    /// # Brief
    /// 1. Create a `OperationType` enum.
    /// 2. Serialize the enum.
    /// 3. Check if it is correct.
    #[test]
    fn ut_operation_type_serialize() {
        let normal = AssetStatus::Normal;
        let insert = AssetStatus::Insert;
        let update = AssetStatus::Update;
        let delete = AssetStatus::Delete;
        let abnormal = AssetStatus::Abnormal;
        let downloading = AssetStatus::Downloading;

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&normal).unwrap();
        msg_parcel.write(&insert).unwrap();
        msg_parcel.write(&update).unwrap();
        msg_parcel.write(&delete).unwrap();
        msg_parcel.write(&abnormal).unwrap();
        msg_parcel.write(&downloading).unwrap();

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 0_u32);
        assert_eq!(msg_parcel.read::<u32>().unwrap(), 1_u32);
        assert_eq!(msg_parcel.read::<u32>().unwrap(), 2_u32);
        assert_eq!(msg_parcel.read::<u32>().unwrap(), 3_u32);
        assert_eq!(msg_parcel.read::<u32>().unwrap(), 4_u32);
        assert_eq!(msg_parcel.read::<u32>().unwrap(), 5_u32);
    }

    /// UT test for operation_type_deserialize.
    ///
    /// # Title
    /// ut_operation_type_deserialize
    ///
    /// # Brief
    /// 1. Create a `OperationType` enum.
    /// 2. Serialize the enum.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `OperationType`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_operation_type_deserialize() {
        let normal = AssetStatus::Normal;
        let insert = AssetStatus::Insert;
        let update = AssetStatus::Update;
        let delete = AssetStatus::Delete;
        let abnormal = AssetStatus::Abnormal;
        let downloading = AssetStatus::Downloading;

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&normal).unwrap();
        msg_parcel.write(&insert).unwrap();
        msg_parcel.write(&update).unwrap();
        msg_parcel.write(&delete).unwrap();
        msg_parcel.write(&abnormal).unwrap();
        msg_parcel.write(&downloading).unwrap();

        assert_eq!(msg_parcel.read::<AssetStatus>().unwrap(), normal);
        assert_eq!(msg_parcel.read::<AssetStatus>().unwrap(), insert);
        assert_eq!(msg_parcel.read::<AssetStatus>().unwrap(), update);
        assert_eq!(msg_parcel.read::<AssetStatus>().unwrap(), delete);
        assert_eq!(msg_parcel.read::<AssetStatus>().unwrap(), abnormal);
        assert_eq!(msg_parcel.read::<AssetStatus>().unwrap(), downloading);
    }

    /// UT test for switch_status_serialize.
    ///
    /// # Title
    /// ut_switch_status_serialize
    ///
    /// # Brief
    /// 1. Create a `SwitchStatus` enum.
    /// 2. Serialize the enum.
    /// 3. Check if it is correct.
    #[test]
    fn ut_switch_status_serialize() {
        let close = SwitchStatus::Close;
        let open = SwitchStatus::Open;
        let not_enable = SwitchStatus::NotEnable;

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&close).unwrap();
        msg_parcel.write(&open).unwrap();
        msg_parcel.write(&not_enable).unwrap();

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 0_u32);
        assert_eq!(msg_parcel.read::<u32>().unwrap(), 1_u32);
        assert_eq!(msg_parcel.read::<u32>().unwrap(), 2_u32);
    }

    /// UT test for switch_status_deserialize.
    ///
    /// # Title
    /// ut_switch_status_deserialize
    ///
    /// # Brief
    /// 1. Create a `SwitchStatus` enum.
    /// 2. Serialize the enum.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `SwitchStatus`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_switch_status_deserialize() {
        let close = SwitchStatus::Close;
        let open = SwitchStatus::Open;
        let not_enable = SwitchStatus::NotEnable;

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&close).unwrap();
        msg_parcel.write(&open).unwrap();
        msg_parcel.write(&not_enable).unwrap();

        assert_eq!(msg_parcel.read::<SwitchStatus>().unwrap(), close);
        assert_eq!(msg_parcel.read::<SwitchStatus>().unwrap(), open);
        assert_eq!(msg_parcel.read::<SwitchStatus>().unwrap(), not_enable);
    }

    /// UT test for asset_serialize.
    ///
    /// # Title
    /// ut_asset_serialize
    ///
    /// # Brief
    /// 1. Create a `CloudAsset` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_asset_serialize() {
        let asset = CloudAsset::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&asset).unwrap();

        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(
            msg_parcel.read::<AssetStatus>().unwrap(),
            AssetStatus::Normal
        );
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
    }

    /// UT test for asset_deserialize.
    ///
    /// # Title
    /// ut_asset_deserialize
    ///
    /// # Brief
    /// 1. Create a `CloudAsset` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `CloudAsset`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_asset_deserialize() {
        let asset = CloudAsset::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&asset).unwrap();

        let result = msg_parcel.read::<CloudAsset>().unwrap();
        assert_eq!(result.uri, asset.uri);
        assert_eq!(result.asset_name, asset.asset_name);
        assert_eq!(result.status, asset.status);
        assert_eq!(result.create_time, asset.create_time);
        assert_eq!(result.modify_time, asset.modify_time);
        assert_eq!(result.size, asset.size);
        assert_eq!(result.sub_path, asset.sub_path);
        assert_eq!(result.hash, asset.hash);
        assert_eq!(result.asset_id, asset.asset_id);
    }

    /// UT test for assets_serialize.
    ///
    /// # Title
    /// ut_assets_serialize
    ///
    /// # Brief
    /// 1. Create a `CloudAssets` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_assets_serialize() {
        let mut assets = CloudAssets::default();
        let asset_one = CloudAsset::default();
        let asset_two = CloudAsset::default();
        assets.0.push(asset_one);
        assets.0.push(asset_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&assets).unwrap();

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 2);

        let default_one = CloudAsset::default();
        let result_one = msg_parcel.read::<CloudAsset>().unwrap();
        assert_eq!(result_one.uri, default_one.uri);
        assert_eq!(result_one.asset_name, default_one.asset_name);
        assert_eq!(result_one.status, default_one.status);
        assert_eq!(result_one.create_time, default_one.create_time);
        assert_eq!(result_one.modify_time, default_one.modify_time);
        assert_eq!(result_one.size, default_one.size);
        assert_eq!(result_one.sub_path, default_one.sub_path);
        assert_eq!(result_one.hash, default_one.hash);
        assert_eq!(result_one.asset_id, default_one.asset_id);

        let default_two = CloudAsset::default();
        let result_two = msg_parcel.read::<CloudAsset>().unwrap();
        assert_eq!(result_two.uri, default_two.uri);
        assert_eq!(result_two.asset_name, default_two.asset_name);
        assert_eq!(result_two.status, default_two.status);
        assert_eq!(result_two.create_time, default_two.create_time);
        assert_eq!(result_two.modify_time, default_two.modify_time);
        assert_eq!(result_two.size, default_two.size);
        assert_eq!(result_two.sub_path, default_two.sub_path);
        assert_eq!(result_two.hash, default_two.hash);
        assert_eq!(result_two.asset_id, default_two.asset_id);
    }

    /// UT test for assets_deserialize.
    ///
    /// # Title
    /// ut_assets_deserialize
    ///
    /// # Brief
    /// 1. Create a `CloudAssets` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `CloudAssets`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_assets_deserialize() {
        let mut assets = CloudAssets::default();
        let asset_one = CloudAsset::default();
        let asset_two = CloudAsset::default();
        assets.0.push(asset_one);
        assets.0.push(asset_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&assets).unwrap();

        assert!(msg_parcel.read::<CloudAssets>().is_ok());
    }

    /// UT test for field_serialize.
    ///
    /// # Title
    /// ut_field_serialize
    ///
    /// # Brief
    /// 1. Create a `Field` enum.
    /// 2. Serialize the enum.
    /// 3. Check if it is correct.
    #[test]
    fn ut_field_serialize() {
        let null = FieldRaw::Null;
        let number = FieldRaw::Number(1);
        let real = FieldRaw::Real(2.0);
        let text = FieldRaw::Text(String::from("text"));
        let boolean = FieldRaw::Bool(true);
        let blob = FieldRaw::Blob(vec![0]);
        let asset = FieldRaw::Asset(CloudAsset::default());
        let assets = FieldRaw::Assets(CloudAssets::default());

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&null).unwrap();
        msg_parcel.write(&number).unwrap();
        msg_parcel.write(&real).unwrap();
        msg_parcel.write(&text).unwrap();
        msg_parcel.write(&boolean).unwrap();
        msg_parcel.write(&blob).unwrap();
        msg_parcel.write(&asset).unwrap();
        msg_parcel.write(&assets).unwrap();

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 0);

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 1);
        assert_eq!(msg_parcel.read::<i64>().unwrap(), 1);

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 2);
        assert_eq!(msg_parcel.read::<f64>().unwrap(), 2.0);

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 3);
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from("text"));

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 4);
        assert!(msg_parcel.read::<bool>().unwrap());

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 5);
        assert_eq!(msg_parcel.read::<Vec<u8>>().unwrap(), vec![0]);

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 6);
        assert!(msg_parcel.read::<CloudAsset>().is_ok());

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 7);
        assert!(msg_parcel.read::<CloudAssets>().is_ok());
    }

    /// UT test for field_deserialize.
    ///
    /// # Title
    /// ut_field_deserialize
    ///
    /// # Brief
    /// 1. Create a `Field` enum.
    /// 2. Serialize the enum.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `Field`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_field_deserialize() {
        let null = FieldRaw::Null;
        let number = FieldRaw::Number(1);
        let text = FieldRaw::Text(String::from("text"));
        let boolean = FieldRaw::Bool(true);
        let blob = FieldRaw::Blob(vec![0]);
        let asset = FieldRaw::Asset(CloudAsset::default());
        let assets = FieldRaw::Assets(CloudAssets::default());

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&null).unwrap();
        msg_parcel.write(&number).unwrap();
        msg_parcel.write(&text).unwrap();
        msg_parcel.write(&boolean).unwrap();
        msg_parcel.write(&blob).unwrap();
        msg_parcel.write(&asset).unwrap();
        msg_parcel.write(&assets).unwrap();

        assert!(msg_parcel.read::<FieldRaw>().is_ok());
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
    }

    /// UT test for value_bucket_data_serialize.
    ///
    /// # Title
    /// ut_value_bucket_data_serialize
    ///
    /// # Brief
    /// 1. Create a `ValueBucketData` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_value_bucket_data_serialize() {
        let mut value_bucket_data = ValueBucket::default();
        value_bucket_data
            .0
            .insert(String::from("key1"), FieldRaw::Null);
        value_bucket_data
            .0
            .insert(String::from("key2"), FieldRaw::Number(1));

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&value_bucket_data).unwrap();

        assert_eq!(msg_parcel.read::<i32>().unwrap(), 2);
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from("key1"));
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from("key2"));
        assert!(msg_parcel.read::<FieldRaw>().is_ok());
    }

    /// UT test for value_bucket_data_deserialize.
    ///
    /// # Title
    /// ut_value_bucket_data_deserialize
    ///
    /// # Brief
    /// 1. Create a `ValueBucketData` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `ValueBucketData`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_value_bucket_data_deserialize() {
        let mut value_bucket_data = ValueBucket::default();
        value_bucket_data
            .0
            .insert(String::from("key1"), FieldRaw::Null);
        value_bucket_data
            .0
            .insert(String::from("key2"), FieldRaw::Number(1));

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&value_bucket_data).unwrap();

        assert!(msg_parcel.read::<ValueBucket>().is_ok());
    }

    /// UT test for value_bucket_serialize.
    ///
    /// # Title
    /// ut_value_bucket_serialize
    ///
    /// # Brief
    /// 1. Create a `ValueBucket` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_value_bucket_serialize() {
        let value_bucket = ValueBucket::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&value_bucket).unwrap();

        assert_eq!(msg_parcel.read::<i32>().unwrap(), 0);
    }

    /// UT test for value_bucket_deserialize.
    ///
    /// # Title
    /// ut_value_bucket_deserialize
    ///
    /// # Brief
    /// 1. Create a `ValueBucket` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `ValueBucket`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_value_bucket_deserialize() {
        let value_bucket = ValueBucket::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&value_bucket).unwrap();

        assert!(msg_parcel.read::<ValueBucket>().is_ok());
    }

    /// UT test for value_buckets_serialize.
    ///
    /// # Title
    /// ut_value_buckets_serialize
    ///
    /// # Brief
    /// 1. Create a `ValueBuckets` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_value_buckets_serialize() {
        let mut value_buckets = ValueBuckets::default();
        let value_bucket_one = ValueBucket::default();
        let value_bucket_two = ValueBucket::default();
        value_buckets.0.push(value_bucket_one);
        value_buckets.0.push(value_bucket_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&value_buckets).unwrap();

        assert_eq!(msg_parcel.read::<u32>().unwrap(), 2);
        for _ in 0..2 {
            assert!(msg_parcel.read::<ValueBucket>().is_ok());
        }
    }

    /// UT test for value_buckets_deserialize.
    ///
    /// # Title
    /// ut_value_buckets_deserialize
    ///
    /// # Brief
    /// 1. Create a `ValueBuckets` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `ValueBuckets`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_value_buckets_deserialize() {
        let mut value_buckets = ValueBuckets::default();
        let value_bucket_one = ValueBucket::default();
        let value_bucket_two = ValueBucket::default();
        value_buckets.0.push(value_bucket_one);
        value_buckets.0.push(value_bucket_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&value_buckets).unwrap();

        assert!(msg_parcel.read::<ValueBuckets>().is_ok());
    }

    /// UT test for cloud_data_deserialize.
    ///
    /// # Title
    /// ut_cloud_data_deserialize
    ///
    /// # Brief
    /// 1. Create a `CloudData` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `CloudData`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_cloud_data_deserialize() {
        let next_cursor = String::from("");
        let has_more = false;
        let values = ValueBuckets::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write_string16(&next_cursor).unwrap();
        msg_parcel.write(&has_more).unwrap();
        msg_parcel.write(&values).unwrap();

        assert!(msg_parcel.read::<CloudData>().is_ok());
    }

    /// UT test for database_serialize.
    ///
    /// # Title
    /// ut_database_serialize
    ///
    /// # Brief
    /// 1. Create a `Database` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_database_serialize() {
        let database = Database::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&database).unwrap();

        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(msg_parcel.read_string16().unwrap(), String::from(""));
        assert_eq!(
            msg_parcel.read::<SchemaOrderTables>().unwrap(),
            SchemaOrderTables::default()
        );
    }

    /// UT test for database_deserialize.
    ///
    /// # Title
    /// ut_database_deserialize
    ///
    /// # Brief
    /// 1. Create a `Database` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `Database`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_database_deserialize() {
        let database = Database::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&database).unwrap();

        assert!(msg_parcel.read::<Database>().is_ok());
    }

    /// UT test for order_table_serialize.
    ///
    /// # Title
    /// ut_order_table_serialize
    ///
    /// # Brief
    /// 1. Create a `OrderTable` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_order_table_serialize() {
        let order_table = OrderTable::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&order_table).unwrap();

        assert_eq!(msg_parcel.read_string16().unwrap(), String::default());
        assert_eq!(msg_parcel.read_string16().unwrap(), String::default());
    }

    /// UT test for order_table_deserialize.
    ///
    /// # Title
    /// ut_order_table_deserialize
    ///
    /// # Brief
    /// 1. Create a `OrderTable` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `OrderTable`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_order_table_deserialize() {
        let order_table = OrderTable::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&order_table).unwrap();

        assert_eq!(msg_parcel.read::<OrderTable>().unwrap(), order_table);
    }

    /// UT test for schema_order_tables_serialize.
    ///
    /// # Title
    /// ut_schema_order_tables_serialize
    ///
    /// # Brief
    /// 1. Create a `SchemaOrderTables` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_schema_order_tables_serialize() {
        let mut schema_order_tables = SchemaOrderTables::default();
        let order_table_one = OrderTable::default();
        let order_table_two = OrderTable::default();
        schema_order_tables.0.push(order_table_one);
        schema_order_tables.0.push(order_table_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&schema_order_tables).unwrap();

        assert_eq!(msg_parcel.read::<i32>().unwrap(), 2);
        assert_eq!(
            msg_parcel.read::<OrderTable>().unwrap(),
            OrderTable::default()
        );
        assert_eq!(
            msg_parcel.read::<OrderTable>().unwrap(),
            OrderTable::default()
        );
    }

    /// UT test for schema_order_tables_deserialize.
    ///
    /// # Title
    /// ut_schema_order_tables_deserialize
    ///
    /// # Brief
    /// 1. Create a `SchemaOrderTables` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `SchemaOrderTables`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_schema_order_tables_deserialize() {
        let mut schema_order_tables = SchemaOrderTables::default();
        let order_table_one = OrderTable::default();
        let order_table_two = OrderTable::default();
        schema_order_tables.0.push(order_table_one);
        schema_order_tables.0.push(order_table_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&schema_order_tables).unwrap();

        assert_eq!(
            msg_parcel.read::<SchemaOrderTables>().unwrap(),
            schema_order_tables
        );
    }

    /// UT test for databases_serialize.
    ///
    /// # Title
    /// ut_databases_serialize
    ///
    /// # Brief
    /// 1. Create a `Databases` struct.
    /// 2. Serialize the struct.
    /// 3. Check if it is correct.
    #[test]
    fn ut_databases_serialize() {
        let mut databases = Databases::default();
        let database_one = Database::default();
        let database_two = Database::default();
        databases.0.push(database_one);
        databases.0.push(database_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&databases).unwrap();

        assert_eq!(msg_parcel.read::<i32>().unwrap(), 2);
        assert!(msg_parcel.read::<Database>().is_ok());
        assert!(msg_parcel.read::<Database>().is_ok());
    }

    /// UT test for databases_deserialize.
    ///
    /// # Title
    /// ut_databases_deserialize
    ///
    /// # Brief
    /// 1. Create a `Databases` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `Databases`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_databases_deserialize() {
        let mut databases = Databases::default();
        let database_one = Database::default();
        let database_two = Database::default();
        databases.0.push(database_one);
        databases.0.push(database_two);

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&databases).unwrap();

        assert!(msg_parcel.read::<Databases>().is_ok());
    }

    /// UT test for schema_deserialize.
    ///
    /// # Title
    /// ut_schema_deserialize
    ///
    /// # Brief
    /// 1. Create a `Schema` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `Schema`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_schema_deserialize() {
        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&0_i32).unwrap();
        msg_parcel.write_string16("").unwrap();
        msg_parcel.write(&Databases::default()).unwrap();

        assert!(msg_parcel.read::<Schema>().is_ok());
    }

    /// UT test for app_info_deserialize.
    ///
    /// # Title
    /// ut_app_info_deserialize
    ///
    /// # Brief
    /// 1. Create a `AppInfo` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `AppInfo`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_app_info_deserialize() {
        let app_info = AppInfo::default();

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write_string16("").unwrap();
        msg_parcel.write_string16("").unwrap();
        msg_parcel.write(&bool::default()).unwrap();
        msg_parcel.write(&0_i32).unwrap();
        msg_parcel.write(&0_i32).unwrap();

        assert_eq!(msg_parcel.read::<AppInfo>().unwrap(), app_info);
    }

    /// UT test for subscription_result_value_deserialize.
    ///
    /// # Title
    /// ut_subscription_result_value_deserialize
    ///
    /// # Brief
    /// 1. Create a `SubscriptionResultValue` struct.
    /// 2. Serialize the struct.
    /// 3. Read the data in `MsgParcel`.
    /// 4. Deserialize the data to `SubscriptionResultValue`.
    /// 5. Check if it is correct.
    #[test]
    fn ut_subscription_result_value_deserialize() {
        let mut subscription = SubscriptionResultValue::default();
        subscription
            .0
            .push((String::from("test1"), String::from("2")));

        let mut msg_parcel = MsgParcel::new();
        msg_parcel.write(&1_i32).unwrap();
        msg_parcel.write_string16(&String::from("test1")).unwrap();
        msg_parcel.write_string16(&String::from("2")).unwrap();

        assert_eq!(
            msg_parcel.read::<SubscriptionResultValue>().unwrap(),
            subscription
        );
    }
}
