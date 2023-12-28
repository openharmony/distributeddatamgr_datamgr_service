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

use crate::c_adapter::basic_rust_types::{HashMapCffi, VectorCffi};
use crate::c_adapter::*;
use crate::ipc_conn;
use crate::service_impl::{asset_loader, cloud_db, cloud_service, types};
use std::collections::HashMap;
use std::ffi::{c_int, c_longlong, c_uchar, c_uint, c_ulonglong, c_void};
use std::ptr::{null, null_mut, slice_from_raw_parts};

/// CloudAssetBuilder defined in C struct. It will be used to build a Rust CloudAsset struct in use.
#[repr(C)]
pub struct OhCloudExtCloudAssetBuilder {
    pub version: u64,
    pub status: asset_loader::AssetStatus,
    pub expires_time: u64,
    pub id: *const c_uchar,
    pub id_len: c_uint,
    pub name: *const c_uchar,
    pub name_len: c_uint,
    pub uri: *const c_uchar,
    pub uri_len: c_uint,
    pub local_path: *const c_uchar,
    pub local_path_len: c_uint,
    pub create_time: *const c_uchar,
    pub create_time_len: c_uint,
    pub modify_time: *const c_uchar,
    pub modify_time_len: c_uint,
    pub size: *const c_uchar,
    pub size_len: c_uint,
    pub hash: *const c_uchar,
    pub hash_len: c_uint,
}

/// Enumeration represents inner type of contents in Value enum.
#[repr(C)]
#[allow(non_camel_case_types, clippy::upper_case_acronyms)]
#[derive(Debug, Eq, PartialEq)]
pub enum OhCloudExtValueType {
    EMPTY = 0,
    INT,
    FLOAT,
    STRING,
    BOOL,
    BYTES,
    ASSET,
    ASSETS,
}

/// Value pointer passed to C side.
pub type OhCloudExtValue = SafeCffiWrapper<types::Value>;
/// CloudAsset pointer passed to C side.
pub type OhCloudExtCloudAsset = SafeCffiWrapper<ipc_conn::CloudAsset>;
/// Database pointer passed to C side.
pub type OhCloudExtDatabase = SafeCffiWrapper<types::Database>;
/// CloudInfo pointer passed to C side.
pub type OhCloudExtCloudInfo = SafeCffiWrapper<cloud_service::CloudInfo>;
/// AppInfo pointer passed to C side.
pub type OhCloudExtAppInfo = SafeCffiWrapper<cloud_service::AppInfo>;
/// Table pointer passed to C side.
pub type OhCloudExtTable = SafeCffiWrapper<types::Table>;
/// Field pointer passed to C side.
pub type OhCloudExtField = SafeCffiWrapper<types::Field>;
/// SchemaMeta pointer passed to C side.
pub type OhCloudExtSchemaMeta = SafeCffiWrapper<cloud_service::SchemaMeta>;
/// RelationSet pointer passed to C side.
pub type OhCloudExtRelationSet = SafeCffiWrapper<cloud_service::RelationSet>;
/// CloudDbData pointer passed to C side.
pub type OhCloudExtCloudDbData = SafeCffiWrapper<cloud_db::CloudDbData>;
/// ValueBucket pointer passed to C side.
pub type OhCloudExtValueBucket = SafeCffiWrapper<ipc_conn::ValueBucket>;

/// Create a Value instance according to ValueInnerType.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtValueNew(
    typ: OhCloudExtValueType,
    content: *mut c_void,
    len: c_uint,
) -> *mut OhCloudExtValue {
    if typ != OhCloudExtValueType::EMPTY && content.is_null() {
        return null_mut();
    }

    let val = match typ {
        OhCloudExtValueType::EMPTY => types::Value::Empty,
        OhCloudExtValueType::INT => {
            let int = &*(content as *mut i64);
            types::Value::Int(*int)
        }
        OhCloudExtValueType::FLOAT => {
            let float = &*(content as *mut f64);
            types::Value::Float(*float)
        }
        OhCloudExtValueType::BOOL => {
            let bool = &*(content as *mut u8);
            types::Value::Bool(c_bool_to_bool(*bool))
        }
        OhCloudExtValueType::STRING => {
            let str = char_ptr_to_string(content as *const c_uchar, len);
            types::Value::String(str)
        }
        OhCloudExtValueType::BYTES => {
            let slice = &(*slice_from_raw_parts(content as *const u8, len as usize));
            types::Value::Bytes(slice.to_vec())
        }
        OhCloudExtValueType::ASSET => {
            let asset = match OhCloudExtCloudAsset::get_inner(
                content as *mut OhCloudExtCloudAsset,
                SafetyCheckId::CloudAsset,
            ) {
                None => return null_mut(),
                Some(v) => v,
            };
            types::Value::Asset(asset)
        }
        OhCloudExtValueType::ASSETS => {
            let assets = match OhCloudExtVector::get_inner(
                content as *mut OhCloudExtVector,
                SafetyCheckId::Vector,
            ) {
                None => return null_mut(),
                Some(v) => v,
            };
            match assets {
                VectorCffi::CloudAsset(v) => types::Value::Assets(v),
                _ => return null_mut(),
            }
        }
    };

    OhCloudExtValue::new(val, SafetyCheckId::Value).into_ptr() as *mut OhCloudExtValue
}

/// Get content from a Value pointer.
///
/// If the Value is Value::Assets or Value::Asset, the Vector pointer returned should be freed by
/// `VectorFree`.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtValueGetContent(
    src: *mut OhCloudExtValue,
    typ: *mut OhCloudExtValueType,
    content: *mut *const c_void,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || typ.is_null() || content.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let value_struct = match OhCloudExtValue::get_inner_ref(src, SafetyCheckId::Value) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match value_struct {
        types::Value::Empty => {
            *typ = OhCloudExtValueType::EMPTY;
            *content = null();
            *len = 0;
        }
        types::Value::Int(c) => {
            *typ = OhCloudExtValueType::INT;
            *content = c as *const _ as *const c_void;
        }
        types::Value::Float(c) => {
            *typ = OhCloudExtValueType::FLOAT;
            *content = c as *const _ as *const c_void;
        }
        types::Value::String(c) => {
            *typ = OhCloudExtValueType::STRING;
            *content = c.as_ptr() as *const c_void;
            *len = c.len() as c_uint;
        }
        types::Value::Bool(c) => {
            *typ = OhCloudExtValueType::BOOL;
            *content = c as *const _ as *const c_void;
        }
        types::Value::Bytes(c) => {
            *typ = OhCloudExtValueType::BYTES;
            *content = c.as_ptr() as *const c_void;
            *len = c.len() as c_uint;
        }
        types::Value::Asset(c) => {
            *typ = OhCloudExtValueType::ASSET;
            *content = OhCloudExtCloudAsset::new(c.clone(), SafetyCheckId::CloudAsset).into_ptr()
                as *mut c_void;
        }
        types::Value::Assets(c) => {
            *typ = OhCloudExtValueType::ASSETS;
            let vec = VectorCffi::CloudAsset(c.to_vec());
            *content = OhCloudExtVector::new(vec, SafetyCheckId::Vector).into_ptr() as *mut c_void;
            *len = c.len() as c_uint;
        }
    }
    ERRNO_SUCCESS
}

/// Free a Value pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtValueFree(src: *mut OhCloudExtValue) {
    let _ = OhCloudExtValue::from_ptr(src, SafetyCheckId::Value);
}

unsafe fn get_cloud_asset(builder: *const OhCloudExtCloudAssetBuilder) -> ipc_conn::CloudAsset {
    let builder = &*builder;
    let id = char_ptr_to_string(builder.id, builder.id_len);
    let name = char_ptr_to_string(builder.name, builder.name_len);
    let uri = char_ptr_to_string(builder.uri, builder.uri_len);
    let local_path = char_ptr_to_string(builder.local_path, builder.local_path_len);
    let create_time = char_ptr_to_string(builder.create_time, builder.create_time_len);
    let modify_time = char_ptr_to_string(builder.modify_time, builder.modify_time_len);
    let size = char_ptr_to_string(builder.size, builder.size_len);
    let hash = char_ptr_to_string(builder.hash, builder.hash_len);
    ipc_conn::CloudAsset {
        asset_id: id,
        status: builder.status.clone(),
        asset_name: name,
        hash,
        uri,
        sub_path: local_path,
        create_time,
        modify_time,
        size,
    }
}

/// Initialize an CloudAsset by the CloudAssetBuilder.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetNew(
    builder: *const OhCloudExtCloudAssetBuilder,
) -> *mut OhCloudExtCloudAsset {
    if builder.is_null() {
        return null_mut();
    }

    let cloud_asset = get_cloud_asset(builder);
    OhCloudExtCloudAsset::new(cloud_asset, SafetyCheckId::CloudAsset).into_ptr()
        as *mut OhCloudExtCloudAsset
}

/// Get id from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetId(
    src: *const OhCloudExtCloudAsset,
    id: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || id.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *id = src_struct.id().as_ptr() as *const c_uchar;
    *len = src_struct.id().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get name from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetName(
    src: *const OhCloudExtCloudAsset,
    name: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || name.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *name = src_struct.name().as_ptr() as *const c_uchar;
    *len = src_struct.name().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get uri from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetUri(
    src: *const OhCloudExtCloudAsset,
    uri: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || uri.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *uri = src_struct.uri().as_ptr() as *const c_uchar;
    *len = src_struct.uri().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get local path from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetLocalPath(
    src: *const OhCloudExtCloudAsset,
    local_path: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || local_path.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *local_path = src_struct.local_path().as_ptr() as *const c_uchar;
    *len = src_struct.local_path().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get create time from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetCreateTime(
    src: *const OhCloudExtCloudAsset,
    create_time: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || create_time.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *create_time = src_struct.create_time().as_ptr() as *const c_uchar;
    *len = src_struct.create_time().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get modified time from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetModifiedTime(
    src: *const OhCloudExtCloudAsset,
    modify_time: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || modify_time.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *modify_time = src_struct.modify_time().as_ptr() as *const c_uchar;
    *len = src_struct.modify_time().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get size from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetSize(
    src: *const OhCloudExtCloudAsset,
    size: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || size.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *size = src_struct.size().as_ptr() as *const c_uchar;
    *len = src_struct.size().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get hash from an CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetGetHash(
    src: *const OhCloudExtCloudAsset,
    hash: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if src.is_null() || hash.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let src_struct = match OhCloudExtCloudAsset::get_inner_ref(src, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *hash = src_struct.hash().as_ptr() as *const c_uchar;
    *len = src_struct.hash().len() as c_uint;
    ERRNO_SUCCESS
}

/// Free a CloudAsset pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetFree(src: *mut OhCloudExtCloudAsset) {
    let _ = OhCloudExtCloudAsset::from_ptr(src, SafetyCheckId::CloudAsset);
}

/// Create a Database instance by database name, alias, and tables. Tables can be created by
/// basic_rust_types::HashMap, and its type should be HashMap<String, Table>. When passed in,
/// tables don't need to be freed again, because their management will be transferred to Database
/// instance.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtDatabaseNew(
    name: *const c_uchar,
    name_len: c_uint,
    alias: *const c_uchar,
    alias_len: c_uint,
    tables: *mut OhCloudExtHashMap,
) -> *mut OhCloudExtDatabase {
    let name = char_ptr_to_string(name, name_len);
    let alias = char_ptr_to_string(alias, alias_len);

    if tables.is_null() {
        let t = HashMap::new();
        let db = types::Database::new(name, alias, t);
        OhCloudExtDatabase::new(db, SafetyCheckId::Database).into_ptr() as *mut OhCloudExtDatabase
    } else {
        let tables = match OhCloudExtHashMap::get_inner(tables, SafetyCheckId::HashMap) {
            None => return null_mut(),
            Some(t) => t,
        };
        match tables {
            HashMapCffi::Table(t) => {
                let db = types::Database::new(name, alias, t);
                OhCloudExtDatabase::new(db, SafetyCheckId::Database).into_ptr()
                    as *mut OhCloudExtDatabase
            }
            _ => null_mut(),
        }
    }
}

/// Get name from a Database pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtDatabaseGetName(
    db: *const OhCloudExtDatabase,
    name: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if db.is_null() || name.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let db_struct = match OhCloudExtDatabase::get_inner_ref(db, SafetyCheckId::Database) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *name = db_struct.name.as_ptr() as *const c_uchar;
    *len = db_struct.name.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get alias from a Database pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtDatabaseGetAlias(
    db: *const OhCloudExtDatabase,
    alias: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if db.is_null() || alias.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let db_struct = match OhCloudExtDatabase::get_inner_ref(db, SafetyCheckId::Database) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *alias = db_struct.alias.as_ptr() as *const c_uchar;
    *len = db_struct.alias.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get hashmap of tables from a Database pointer. Parameter `tables` will be updated
/// to hold the output HashMap, and it should be freed by `HashMapFree`.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtDatabaseGetTable(
    db: *const OhCloudExtDatabase,
    tables: *mut *const OhCloudExtHashMap,
) -> c_int {
    if db.is_null() || tables.is_null() {
        return ERRNO_NULLPTR;
    }

    let db_struct = match OhCloudExtDatabase::get_inner_ref(db, SafetyCheckId::Database) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let tb_hashmap = HashMapCffi::Table(db_struct.tables.clone());
    *tables = OhCloudExtHashMap::new(tb_hashmap, SafetyCheckId::HashMap).into_ptr()
        as *const OhCloudExtHashMap;
    ERRNO_SUCCESS
}

/// Free a Database pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtDatabaseFree(src: *mut OhCloudExtDatabase) {
    let _ = OhCloudExtDatabase::from_ptr(src, SafetyCheckId::Database);
}

/// Create a Table instance by table name, table alias, and vector of fields. Fields can be created
/// by basic_rust_types::Vector, and its type should be Vec<Field>. When passed in, fields don't
/// need to be freed again, because their management will be transferred to Table instance.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtTableNew(
    name: *const c_uchar,
    name_len: c_uint,
    alias: *const c_uchar,
    alias_len: c_uint,
    fields: *mut OhCloudExtVector,
) -> *mut OhCloudExtTable {
    let name = char_ptr_to_string(name, name_len);
    let alias = char_ptr_to_string(alias, alias_len);

    if fields.is_null() {
        let f = vec![];
        let tb = types::Table::new(name, alias, f);
        OhCloudExtTable::new(tb, SafetyCheckId::Table).into_ptr() as *mut OhCloudExtTable
    } else {
        let fields = match OhCloudExtVector::get_inner(fields, SafetyCheckId::Vector) {
            None => return null_mut(),
            Some(v) => v,
        };
        match fields {
            VectorCffi::Field(f) => {
                let tb = types::Table::new(name, alias, f);
                OhCloudExtTable::new(tb, SafetyCheckId::Table).into_ptr()
            }
            _ => null_mut(),
        }
    }
}

/// Get name from a Table pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtTableGetName(
    tb: *const OhCloudExtTable,
    name: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if tb.is_null() || name.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let tb_struct = match OhCloudExtTable::get_inner_ref(tb, SafetyCheckId::Table) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *name = tb_struct.name.as_ptr() as *const c_uchar;
    *len = tb_struct.name.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get alias from a Table pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtTableGetAlias(
    tb: *const OhCloudExtTable,
    alias: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if tb.is_null() || alias.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let tb_struct = match OhCloudExtTable::get_inner_ref(tb, SafetyCheckId::Table) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *alias = tb_struct.alias.as_ptr() as *const c_uchar;
    *len = tb_struct.alias.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get vector of fields from a Table pointer. Parameter `fields` be updated
/// to hold the output Vector, and it should be freed by `VectorFree`.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtTableGetFields(
    tb: *const OhCloudExtTable,
    fields: *mut *const OhCloudExtVector,
) -> c_int {
    if tb.is_null() || fields.is_null() {
        return ERRNO_NULLPTR;
    }

    let tb_struct = match OhCloudExtTable::get_inner_ref(tb, SafetyCheckId::Table) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let vec = VectorCffi::Field(tb_struct.fields.clone());
    *fields = OhCloudExtVector::new(vec, SafetyCheckId::Vector).into_ptr();
    ERRNO_SUCCESS
}

/// Free a Table pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtTableFree(src: *mut OhCloudExtTable) {
    let _ = OhCloudExtTable::from_ptr(src, SafetyCheckId::Table);
}

#[inline]
fn c_bool_to_bool(src: u8) -> bool {
    src != 0
}

/// FieldBuilder providing necessary information when create a Field.
#[repr(C)]
pub struct OhCloudExtFieldBuilder {
    col_name: *const c_uchar,
    col_name_len: c_uint,
    alias: *const c_uchar,
    alias_len: c_uint,
    typ: c_uint,
    primary: u8,
    nullable: u8,
}

/// Create a Field by column name, alias, type, primary, and nullable.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtFieldNew(
    builder: *const OhCloudExtFieldBuilder,
) -> *mut OhCloudExtField {
    if builder.is_null() {
        return null_mut();
    }
    let builder = &*builder;
    let col_name = char_ptr_to_string(builder.col_name, builder.col_name_len);
    let alias = char_ptr_to_string(builder.alias, builder.alias_len);
    let typ = builder.typ as u8;
    let primary = c_bool_to_bool(builder.primary);
    let nullable = c_bool_to_bool(builder.nullable);
    let fd = types::Field::new(col_name, alias, typ, primary, nullable);
    OhCloudExtField::new(fd, SafetyCheckId::Field).into_ptr()
}

/// Get column name from a Field pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtFieldGetColName(
    fd: *const OhCloudExtField,
    name: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if fd.is_null() || name.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let fd_struct = match OhCloudExtField::get_inner_ref(fd, SafetyCheckId::Field) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *name = fd_struct.col_name.as_ptr() as *const c_uchar;
    *len = fd_struct.col_name.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get alias from a Field pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtFieldGetAlias(
    fd: *const OhCloudExtField,
    alias: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if fd.is_null() || alias.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let fd_struct = match OhCloudExtField::get_inner_ref(fd, SafetyCheckId::Field) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *alias = fd_struct.alias.as_ptr() as *const c_uchar;
    *len = fd_struct.alias.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get type from a Field pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtFieldGetTyp(
    fd: *const OhCloudExtField,
    typ: *mut c_uint,
) -> c_int {
    if fd.is_null() || typ.is_null() {
        return ERRNO_NULLPTR;
    }

    let fd_struct = match OhCloudExtField::get_inner_ref(fd, SafetyCheckId::Field) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *typ = fd_struct.typ as c_uint;
    ERRNO_SUCCESS
}

/// Check whether the Field is primary.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtFieldGetPrimary(
    fd: *const OhCloudExtField,
    primary: *mut u8,
) -> c_int {
    if fd.is_null() || primary.is_null() {
        return ERRNO_NULLPTR;
    }

    let fd_struct = match OhCloudExtField::get_inner_ref(fd, SafetyCheckId::Field) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *primary = fd_struct.primary as u8;
    ERRNO_SUCCESS
}

/// Check whether the Field is nullable.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtFieldGetNullable(
    fd: *const OhCloudExtField,
    nullable: *mut u8,
) -> c_int {
    if fd.is_null() || nullable.is_null() {
        return ERRNO_NULLPTR;
    }

    let fd_struct = match OhCloudExtField::get_inner_ref(fd, SafetyCheckId::Field) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *nullable = fd_struct.nullable as u8;
    ERRNO_SUCCESS
}

/// Free a Field pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtFieldFree(src: *mut OhCloudExtField) {
    let _ = OhCloudExtField::from_ptr(src, SafetyCheckId::Field);
}

/// Get user from a CloudInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudInfoGetUser(
    info: *const OhCloudExtCloudInfo,
    user: *mut c_int,
) -> c_int {
    if info.is_null() || user.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtCloudInfo::get_inner_ref(info, SafetyCheckId::CloudInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *user = info_struct.user;
    ERRNO_SUCCESS
}

/// Get id from a CloudInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudInfoGetId(
    info: *const OhCloudExtCloudInfo,
    id: *mut *const c_uchar,
    id_len: *mut c_uint,
) -> c_int {
    if info.is_null() || id.is_null() || id_len.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtCloudInfo::get_inner_ref(info, SafetyCheckId::CloudInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *id = info_struct.id.as_ptr() as *const c_uchar;
    *id_len = info_struct.id.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get total space from a CloudInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudInfoGetTotalSpace(
    info: *const OhCloudExtCloudInfo,
    total_space: *mut c_ulonglong,
) -> c_int {
    if info.is_null() || total_space.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtCloudInfo::get_inner_ref(info, SafetyCheckId::CloudInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *total_space = info_struct.total_space as c_ulonglong;
    ERRNO_SUCCESS
}

/// Get remain space from a CloudInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudInfoGetRemainSpace(
    info: *const OhCloudExtCloudInfo,
    remain_space: *mut c_ulonglong,
) -> c_int {
    if info.is_null() || remain_space.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtCloudInfo::get_inner_ref(info, SafetyCheckId::CloudInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *remain_space = info_struct.remain_space as c_ulonglong;
    ERRNO_SUCCESS
}

/// Check whether a CloudInfo enables cloud sync.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudInfoEnabled(
    info: *const OhCloudExtCloudInfo,
    enable: *mut u8,
) -> c_int {
    if info.is_null() || enable.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtCloudInfo::get_inner_ref(info, SafetyCheckId::CloudInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *enable = info_struct.enable_cloud as u8;
    ERRNO_SUCCESS
}

/// Get hashmap of AppInfo from a CloudInfo pointer. Parameter `app_info` will be updated
/// to hold the output HashMap, and it should be freed by `HashMapFree`.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudInfoGetAppInfo(
    info: *const OhCloudExtCloudInfo,
    app_info: *mut *const OhCloudExtHashMap,
) -> c_int {
    if info.is_null() || app_info.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtCloudInfo::get_inner_ref(info, SafetyCheckId::CloudInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let apps = HashMapCffi::AppInfo(info_struct.apps.clone());
    *app_info = OhCloudExtHashMap::new(apps, SafetyCheckId::HashMap).into_ptr();
    ERRNO_SUCCESS
}

/// Free a CloudInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudInfoFree(info: *mut OhCloudExtCloudInfo) {
    let _ = OhCloudExtCloudInfo::from_ptr(info, SafetyCheckId::CloudInfo);
}

/// Get app id from an AppInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtAppInfoGetAppId(
    info: *const OhCloudExtAppInfo,
    id: *mut *const c_uchar,
    id_len: *mut c_uint,
) -> c_int {
    if info.is_null() || id.is_null() || id_len.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtAppInfo::get_inner_ref(info, SafetyCheckId::AppInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *id = info_struct.app_id.as_ptr() as *const c_uchar;
    *id_len = info_struct.app_id.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get bundle name from an AppInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtAppInfoGetBundleName(
    info: *const OhCloudExtAppInfo,
    id: *mut *const c_uchar,
    id_len: *mut c_uint,
) -> c_int {
    if info.is_null() || id.is_null() || id_len.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtAppInfo::get_inner_ref(info, SafetyCheckId::AppInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *id = info_struct.bundle_name.as_ptr() as *const c_uchar;
    *id_len = info_struct.bundle_name.len() as c_uint;
    ERRNO_SUCCESS
}

/// Check whether an AppInfo pointer allows cloud switch.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtAppInfoGetCloudSwitch(
    info: *const OhCloudExtAppInfo,
    switch: *mut u8,
) -> c_int {
    if info.is_null() || switch.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtAppInfo::get_inner_ref(info, SafetyCheckId::AppInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *switch = info_struct.cloud_switch as u8;
    ERRNO_SUCCESS
}

/// Get instance id from an AppInfo pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtAppInfoGetInstanceId(
    info: *const OhCloudExtAppInfo,
    id: *mut c_int,
) -> c_int {
    if info.is_null() || id.is_null() {
        return ERRNO_NULLPTR;
    }

    let info_struct = match OhCloudExtAppInfo::get_inner_ref(info, SafetyCheckId::AppInfo) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *id = info_struct.instance_id;
    ERRNO_SUCCESS
}

/// Free a AppInfo ptr.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtAppInfoFree(info: *mut OhCloudExtAppInfo) {
    let _ = OhCloudExtAppInfo::from_ptr(info, SafetyCheckId::AppInfo);
}

/// Get version from a SchemaMeta pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtSchemaMetaGetVersion(
    schema: *mut OhCloudExtSchemaMeta,
    version: *mut c_int,
) -> c_int {
    if schema.is_null() || version.is_null() {
        return ERRNO_NULLPTR;
    }

    let schema_struct = match OhCloudExtSchemaMeta::get_inner_ref(schema, SafetyCheckId::SchemaMeta)
    {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *version = schema_struct.version();
    ERRNO_SUCCESS
}

/// Get bundle name from a SchemaMeta pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtSchemaMetaGetBundleName(
    schema: *mut OhCloudExtSchemaMeta,
    name: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if schema.is_null() || name.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let schema_struct = match OhCloudExtSchemaMeta::get_inner_ref(schema, SafetyCheckId::SchemaMeta)
    {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *name = schema_struct.bundle_name().as_ptr() as *const c_uchar;
    *len = schema_struct.bundle_name().len() as c_uint;
    ERRNO_SUCCESS
}

/// Get a vector of databases from a SchemaMeta pointer. Parameter `dbs` be updated
/// to hold the output Vector, and it should be freed by `VectorFree`.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtSchemaMetaGetDatabases(
    schema: *mut OhCloudExtSchemaMeta,
    dbs: *mut *const OhCloudExtVector,
) -> c_int {
    if schema.is_null() || dbs.is_null() {
        return ERRNO_NULLPTR;
    }

    let schema_struct = match OhCloudExtSchemaMeta::get_inner_ref(schema, SafetyCheckId::SchemaMeta)
    {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let databases = schema_struct.databases();
    let vec = VectorCffi::Database(databases.to_vec());
    *dbs = OhCloudExtVector::new(vec, SafetyCheckId::Vector).into_ptr();
    ERRNO_SUCCESS
}

/// Free a SchemaMeta pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtSchemaMetaFree(schema: *mut OhCloudExtSchemaMeta) {
    let _ = OhCloudExtSchemaMeta::from_ptr(schema, SafetyCheckId::SchemaMeta);
}

/// Create a RelationSet instance by bundle name, and relations. Relations can be created by
/// basic_rust_types::HashMap, and its type should be HashMap<String, (u32, u32)>. When passed in,
/// tables don't need to be freed again, because their management will be transferred to RelationSet
/// instance.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtRelationSetNew(
    bundle_name: *const c_uchar,
    len: c_uint,
    expire_time: c_longlong,
    relations: *mut OhCloudExtHashMap,
) -> *mut OhCloudExtRelationSet {
    let name = char_ptr_to_string(bundle_name, len);

    if relations.is_null() {
        let re = HashMap::default();
        let relation = cloud_service::RelationSet::new(name, expire_time, re);
        OhCloudExtRelationSet::new(relation, SafetyCheckId::RelationSet).into_ptr()
    } else {
        let relations = match OhCloudExtHashMap::get_inner(relations, SafetyCheckId::HashMap) {
            None => return null_mut(),
            Some(v) => v,
        };
        match relations {
            HashMapCffi::String(re) => {
                let relation = cloud_service::RelationSet::new(name, expire_time, re);
                OhCloudExtRelationSet::new(relation, SafetyCheckId::RelationSet).into_ptr()
            }
            _ => null_mut(),
        }
    }
}

/// Get bundle name from a RelationSet pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtRelationSetGetBundleName(
    relation: *mut OhCloudExtRelationSet,
    bundle_name: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if relation.is_null() || bundle_name.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let relation_struct =
        match OhCloudExtRelationSet::get_inner_ref(relation, SafetyCheckId::RelationSet) {
            None => return ERRNO_WRONG_TYPE,
            Some(v) => v,
        };
    *bundle_name = relation_struct.bundle_name.as_ptr() as *const c_uchar;
    *len = relation_struct.bundle_name.len() as c_uint;
    ERRNO_SUCCESS
}

/// Get expire time from a RelationSet pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtRelationSetGetExpireTime(
    relation: *mut OhCloudExtRelationSet,
    expire_time: *mut c_ulonglong,
) -> c_int {
    if relation.is_null() || expire_time.is_null() {
        return ERRNO_NULLPTR;
    }

    let relation_struct =
        match OhCloudExtRelationSet::get_inner_ref(relation, SafetyCheckId::RelationSet) {
            None => return ERRNO_WRONG_TYPE,
            Some(v) => v,
        };
    *expire_time = relation_struct.expire_time as c_ulonglong;
    ERRNO_SUCCESS
}

/// Get relations from a RelationSet pointer. Parameter `relations` be updated
/// to hold the output HashMap, and it should be freed by `HashMapFree`.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtRelationSetGetRelations(
    relation: *mut OhCloudExtRelationSet,
    relations: *mut *const OhCloudExtHashMap,
) -> c_int {
    if relation.is_null() || relations.is_null() {
        return ERRNO_NULLPTR;
    }

    let relation_struct =
        match OhCloudExtRelationSet::get_inner_ref(relation, SafetyCheckId::RelationSet) {
            None => return ERRNO_WRONG_TYPE,
            Some(v) => v,
        };
    let res = relation_struct.relations();
    let hashmap = HashMapCffi::String(res.clone());
    *relations = OhCloudExtHashMap::new(hashmap, SafetyCheckId::HashMap).into_ptr();
    ERRNO_SUCCESS
}

/// Free a RelationSet pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtRelationSetFree(re: *mut OhCloudExtRelationSet) {
    let _ = OhCloudExtRelationSet::from_ptr(re, SafetyCheckId::RelationSet);
}

/// Get the next cursor from a CloudDbData pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbDataGetNextCursor(
    data: *mut OhCloudExtCloudDbData,
    cursor: *mut *const c_uchar,
    len: *mut c_uint,
) -> c_int {
    if data.is_null() || cursor.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let data_struct = match OhCloudExtCloudDbData::get_inner_ref(data, SafetyCheckId::CloudDbData) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *cursor = data_struct.next_cursor.as_ptr() as *const c_uchar;
    *len = data_struct.next_cursor.len() as c_uint;
    ERRNO_SUCCESS
}

/// Check whether a CloudDbData has more data.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbDataGetHasMore(
    data: *mut OhCloudExtCloudDbData,
    has_more: *mut u8,
) -> c_int {
    if data.is_null() || has_more.is_null() {
        return ERRNO_NULLPTR;
    }

    let data_struct = match OhCloudExtCloudDbData::get_inner_ref(data, SafetyCheckId::CloudDbData) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    *has_more = data_struct.has_more as u8;
    ERRNO_SUCCESS
}

/// Get vector of values from a CloudDbData pointer. Parameter `values` will be updated
/// to hold the output Vec<ValueBucket>, and it should be freed by `VectorFree`.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbDataGetValues(
    data: *mut OhCloudExtCloudDbData,
    values: *mut *const OhCloudExtVector,
) -> c_int {
    if data.is_null() || values.is_null() {
        return ERRNO_NULLPTR;
    }

    let data_struct = match OhCloudExtCloudDbData::get_inner_ref(data, SafetyCheckId::CloudDbData) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let vec = VectorCffi::ValueBucket(data_struct.values.clone());
    *values = OhCloudExtVector::new(vec, SafetyCheckId::Vector).into_ptr();
    ERRNO_SUCCESS
}

/// Free a CloudDbData pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbDataFree(src: *mut OhCloudExtCloudDbData) {
    let _ = OhCloudExtCloudDbData::from_ptr(src, SafetyCheckId::CloudDbData);
}

#[repr(C)]
pub struct OhCloudExtKeyName {
    key: *const c_uchar,
    key_len: c_uint,
}

#[no_mangle]
pub unsafe extern "C" fn OhCloudExtKeyNameNew(key: *const c_uchar, key_len: c_uint) -> OhCloudExtKeyName {
    let key_name = OhCloudExtKeyName { key, key_len };
    key_name
}

/// Get keys from a ValueBucket pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtValueBucketGetKeys(
    vb: *mut OhCloudExtValueBucket,
    keys: *mut *const OhCloudExtVector,
    keys_len: *mut c_uint,
) -> c_int {
    if vb.is_null() || keys.is_null() || keys_len.is_null() {
        return ERRNO_NULLPTR;
    }

    let vb_struct = match OhCloudExtValueBucket::get_inner_ref(vb, SafetyCheckId::ValueBucket) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let keys_vec = vb_struct.0.clone().into_keys().collect::<Vec<String>>();
    *keys_len = keys_vec.len() as c_uint;
    let vec = VectorCffi::String(keys_vec);
    *keys = OhCloudExtVector::new(vec, SafetyCheckId::Vector).into_ptr();
    ERRNO_SUCCESS
}

/// Gets Value from a ValueBucket pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtValueBucketGetValue(
    vb: *mut OhCloudExtValueBucket,
    key_name: OhCloudExtKeyName,
    typ: *mut OhCloudExtValueType,
    content: *mut *const c_void,
    len: *mut c_uint,
) -> c_int {
    let key = key_name.key;
    let key_len = key_name.key_len;

    if vb.is_null() || key.is_null() || typ.is_null() || content.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }

    let name = char_ptr_to_string(key, key_len);

    let vb_struct = match OhCloudExtValueBucket::get_inner_ref(vb, SafetyCheckId::ValueBucket) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let value_struct = match vb_struct.0.get(&name) {
        None => return ERRNO_INVALID_KEY,
        Some(v) => v,
    };

    match value_struct {
        ipc_conn::FieldRaw::Null => {
            *typ = OhCloudExtValueType::EMPTY;
            *content = null();
            *len = 0;
        }
        ipc_conn::FieldRaw::Number(i) => {
            *typ = OhCloudExtValueType::INT;
            *content = i as *const i64 as *const c_void;
        }
        ipc_conn::FieldRaw::Real(f) => {
            *typ = OhCloudExtValueType::FLOAT;
            *content = f as *const f64 as *const c_void;
        }
        ipc_conn::FieldRaw::Text(s) => {
            *typ = OhCloudExtValueType::STRING;
            *content = s.as_ptr() as *const c_void;
            *len = s.len() as c_uint;
        }
        ipc_conn::FieldRaw::Bool(b) => {
            *typ = OhCloudExtValueType::BOOL;
            *content = b as *const bool as *const c_void;
        }
        ipc_conn::FieldRaw::Blob(b) => {
            *typ = OhCloudExtValueType::BYTES;
            *content = b.as_ptr() as *const c_void;
            *len = b.len() as c_uint;
        }
        ipc_conn::FieldRaw::Asset(a) => {
            *typ = OhCloudExtValueType::ASSET;
            *content = OhCloudExtCloudAsset::new(a.clone(), SafetyCheckId::CloudAsset).into_ptr()
                as *mut c_void;
        }
        ipc_conn::FieldRaw::Assets(a) => {
            *typ = OhCloudExtValueType::ASSETS;
            let vec = VectorCffi::CloudAsset(a.0.to_vec());
            *content = OhCloudExtVector::new(vec, SafetyCheckId::Vector).into_ptr() as *mut c_void;
            *len = a.0.len() as c_uint;
        }
    }

    ERRNO_SUCCESS
}

/// Free a ValueBucket ptr.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtValueBucketFree(info: *mut OhCloudExtValueBucket) {
    let _ = OhCloudExtValueBucket::from_ptr(info, SafetyCheckId::ValueBucket);
}

#[cfg(test)]
mod test {
    use crate::c_adapter::basic_rust_types::{
        OhCloudExtHashMap, OhCloudExtRustType, OhCloudExtVector, OhCloudExtVectorNew,
        OhCloudExtVectorPush,
    };
    use crate::c_adapter::cloud_ext_types::*;

    /// UT test for Value creation and destruction.
    ///
    /// # Title
    /// ut_value_int
    ///
    /// # Brief
    /// 1. Create a Value::Int.
    /// 2. Get values from it.
    /// 3. Free the value ptr.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_value_int() {
        unsafe {
            let mut src = 1_i64;
            let value = OhCloudExtValueNew(
                OhCloudExtValueType::INT,
                &mut src as *mut _ as *mut c_void,
                0,
            );
            assert!(!value.is_null());

            let mut typ = OhCloudExtValueType::EMPTY;
            let mut val: *mut c_uint = null_mut();
            let mut length = 1 as c_uint;
            assert_eq!(
                OhCloudExtValueGetContent(
                    value,
                    &mut typ as *mut OhCloudExtValueType,
                    &mut val as *mut _ as *mut *const c_void,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            assert_eq!(*val as i64, src);
            assert_eq!(typ, OhCloudExtValueType::INT);
            OhCloudExtValueFree(value);
        }
    }

    unsafe fn get_asset_ptr() -> *mut OhCloudExtCloudAsset {
        let temp = "temp";
        let mut builder = OhCloudExtCloudAssetBuilder {
            version: 0,
            status: asset_loader::AssetStatus::Normal,
            expires_time: 0,
            id: temp.as_ptr() as *const c_uchar,
            id_len: 4,
            name: temp.as_ptr() as *const c_uchar,
            name_len: 4,
            uri: temp.as_ptr() as *const c_uchar,
            uri_len: 4,
            local_path: temp.as_ptr() as *const c_uchar,
            local_path_len: 4,
            create_time: temp.as_ptr() as *const c_uchar,
            create_time_len: 4,
            modify_time: temp.as_ptr() as *const c_uchar,
            modify_time_len: 4,
            size: temp.as_ptr() as *const c_uchar,
            size_len: 4,
            hash: temp.as_ptr() as *const c_uchar,
            hash_len: 4,
        };
        let asset = OhCloudExtCloudAssetNew(&mut builder as *mut OhCloudExtCloudAssetBuilder);
        assert!(!asset.is_null());
        asset
    }

    /// UT test for Value creation and destruction.
    ///
    /// # Title
    /// ut_value_asset
    ///
    /// # Brief
    /// 1. Create a Value::Asset.
    /// 2. Get values from it.
    /// 3. Free the value ptr.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_value_asset() {
        unsafe {
            let asset = get_asset_ptr();
            let value = OhCloudExtValueNew(OhCloudExtValueType::ASSET, asset as *mut c_void, 0);
            assert!(!value.is_null());

            let mut typ = OhCloudExtValueType::EMPTY;
            let mut val: *mut OhCloudExtCloudAsset = null_mut();
            let mut length = 1 as c_uint;
            assert_eq!(
                OhCloudExtValueGetContent(
                    value,
                    &mut typ as *mut OhCloudExtValueType,
                    &mut val as *mut _ as *mut *const c_void,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            assert!(!val.is_null());
            assert_eq!(typ, OhCloudExtValueType::ASSET);
            OhCloudExtValueFree(value);
        }
    }

    /// UT test for Value creation and destruction.
    ///
    /// # Title
    /// ut_value_assets
    ///
    /// # Brief
    /// 1. Create a Value::Assets.
    /// 2. Get values from it.
    /// 3. Free the value ptr.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_value_assets() {
        unsafe {
            let asset = get_asset_ptr();
            let vec = OhCloudExtVectorNew(OhCloudExtRustType::CLOUD_ASSET);
            assert!(!vec.is_null());
            assert_eq!(
                OhCloudExtVectorPush(vec, asset as *mut c_void, 0),
                ERRNO_SUCCESS
            );
            let value = OhCloudExtValueNew(OhCloudExtValueType::ASSETS, vec as *mut c_void, 0);
            assert!(!value.is_null());

            let mut typ = OhCloudExtValueType::EMPTY;
            let mut val: *mut OhCloudExtCloudAsset = null_mut();
            let mut length = 1 as c_uint;
            assert_eq!(
                OhCloudExtValueGetContent(
                    value,
                    &mut typ as *mut OhCloudExtValueType,
                    &mut val as *mut _ as *mut *const c_void,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            assert!(!val.is_null());
            assert_eq!(typ, OhCloudExtValueType::ASSETS);
            let assets_struct =
                OhCloudExtVector::get_inner(val as *mut OhCloudExtVector, SafetyCheckId::Vector)
                    .unwrap();
            match assets_struct {
                VectorCffi::CloudAsset(v) => {
                    assert_eq!(v.len(), 1);
                }
                _ => panic!("Value typ mismatches with Assets"),
            }
            OhCloudExtValueFree(value);
        }
    }

    /// UT test for Database creation and destruction.
    ///
    /// # Title
    /// ut_database
    ///
    /// # Brief
    /// 1. Create a Database.
    /// 2. Get tables and name from it.
    /// 3. Free the Database ptr.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_database() {
        unsafe {
            let temp = "temp";
            let db = OhCloudExtDatabaseNew(
                temp.as_ptr() as *const c_uchar,
                temp.len() as c_uint,
                null_mut(),
                0,
                null_mut(),
            );
            assert!(!db.is_null());
            let mut name: *mut c_uchar = null_mut();
            let mut length = 1 as c_uint;
            assert_eq!(
                OhCloudExtDatabaseGetAlias(
                    db,
                    &mut name as *mut _ as *mut *const c_uchar,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            let id = &*slice_from_raw_parts(name as *const u8, length as usize);
            assert!(id.is_empty());
            assert_eq!(length, 0);
            let mut map: *mut OhCloudExtHashMap = null_mut();
            assert_eq!(
                OhCloudExtDatabaseGetTable(db, &mut map as *mut _ as *mut *const OhCloudExtHashMap),
                ERRNO_SUCCESS
            );
            assert!(!map.is_null());
            let map_struct = OhCloudExtHashMap::get_inner(map, SafetyCheckId::HashMap).unwrap();
            match map_struct {
                HashMapCffi::Table(h) => {
                    assert!(h.is_empty());
                }
                _ => panic!("Db Table Map mismatches with Table"),
            }
            OhCloudExtDatabaseFree(db);
        }
    }

    /// UT test for Table creation and destruction.
    ///
    /// # Title
    /// ut_table
    ///
    /// # Brief
    /// 1. Create a Table.
    /// 2. Get fields and table name from it.
    /// 3. Free the Table ptr.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_table() {
        unsafe {
            let temp = "temp";
            let tb = OhCloudExtTableNew(
                temp.as_ptr() as *const c_uchar,
                temp.len() as c_uint,
                null_mut(),
                0,
                null_mut(),
            );
            assert!(!tb.is_null());
            let mut name: *mut c_uchar = null_mut();
            let mut length = 1 as c_uint;
            assert_eq!(
                OhCloudExtTableGetName(
                    tb,
                    &mut name as *mut _ as *mut *const c_uchar,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            let tb_name = &*slice_from_raw_parts(name as *const u8, length as usize);
            assert_eq!(tb_name, temp.as_bytes());
            assert_eq!(length as usize, temp.len());
            let mut vec: *mut OhCloudExtVector = null_mut();
            assert_eq!(
                OhCloudExtTableGetFields(tb, &mut vec as *mut _ as *mut *const OhCloudExtVector),
                ERRNO_SUCCESS
            );
            assert!(!vec.is_null());
            let vec_struct = OhCloudExtVector::get_inner(vec, SafetyCheckId::Vector).unwrap();
            match vec_struct {
                VectorCffi::Field(f) => {
                    assert!(f.is_empty());
                }
                _ => panic!("Table fields mismatches with Vec<Field>"),
            }
            OhCloudExtTableFree(tb);
        }
    }

    /// UT test for Field creation and destruction.
    ///
    /// # Title
    /// ut_field
    ///
    /// # Brief
    /// 1. Create a Field.
    /// 2. Get name from it.
    /// 3. Free the Field ptr.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_field() {
        unsafe {
            let temp = "temp";
            let fd = OhCloudExtFieldNew(null_mut());
            assert!(fd.is_null());
            let builder = OhCloudExtFieldBuilder {
                col_name: temp.as_ptr() as *const c_uchar,
                col_name_len: temp.len() as c_uint,
                alias: temp.as_ptr() as *const c_uchar,
                alias_len: temp.len() as c_uint,
                typ: 0,
                primary: 0,
                nullable: 0,
            };
            let fd = OhCloudExtFieldNew(&builder as *const OhCloudExtFieldBuilder);
            assert!(!fd.is_null());

            let mut name: *mut c_uchar = null_mut();
            let mut length = 1 as c_uint;
            assert_eq!(
                OhCloudExtFieldGetAlias(
                    fd,
                    &mut name as *mut _ as *mut *const c_uchar,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            let fd_alias = &*slice_from_raw_parts(name as *const u8, length as usize);
            assert_eq!(fd_alias, temp.as_bytes());
            assert_eq!(length as usize, temp.len());
            let mut bool = true;
            assert_eq!(
                OhCloudExtFieldGetNullable(fd, &mut bool as *mut _ as *mut u8),
                ERRNO_SUCCESS
            );
            assert!(!bool);
            OhCloudExtFieldFree(fd);
        }
    }

    /// UT test for RelationSet creation and destruction.
    ///
    /// # Title
    /// ut_relation_set
    ///
    /// # Brief
    /// 1. Create a RelationSet.
    /// 2. Get name from it.
    /// 3. Free the RelationSet ptr.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_relation_set() {
        unsafe {
            let temp = "temp";
            let set = OhCloudExtRelationSetNew(
                temp.as_ptr() as *const c_uchar,
                temp.len() as c_uint,
                64,
                null_mut(),
            );
            assert!(!set.is_null());

            let mut name: *mut c_uchar = null_mut();
            let mut length = 1 as c_uint;
            assert_eq!(
                OhCloudExtRelationSetGetBundleName(
                    set,
                    &mut name as *mut _ as *mut *const c_uchar,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            let bundle_name = &*slice_from_raw_parts(name as *const u8, length as usize);
            assert_eq!(bundle_name, temp.as_bytes());
            assert_eq!(length as usize, temp.len());
            OhCloudExtRelationSetFree(set);
        }
    }
}
