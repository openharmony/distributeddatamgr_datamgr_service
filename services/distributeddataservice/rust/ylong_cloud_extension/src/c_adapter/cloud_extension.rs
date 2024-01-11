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
use crate::c_adapter::cloud_ext_types::*;
use crate::c_adapter::*;
use crate::ipc_conn;
use crate::service_impl::error::SyncError;
use crate::service_impl::{asset_loader, cloud_db, cloud_service};
use std::ffi::{c_int, c_longlong, c_uchar, c_uint};
use std::io::ErrorKind;
use std::ptr::null_mut;

/// CloudAssetLoader pointer passed to C side.
/// The lifetime 'static is only used to pass compiler check.
pub type OhCloudExtCloudAssetLoader = SafeCffiWrapper<asset_loader::CloudAssetLoader<'static>>;
/// CloudDatabase pointer passed to C side.
pub type OhCloudExtCloudDatabase = SafeCffiWrapper<cloud_db::CloudDatabase>;
/// CloudSync pointer passed to C side.
pub type OhCloudExtCloudSync = SafeCffiWrapper<cloud_service::CloudSync>;

fn map_ipc_err(e: &ipc_conn::Error) -> c_int {
    match e {
        ipc_conn::Error::CreateMsgParcelFailed
        | ipc_conn::Error::GetProxyObjectFailed
        | ipc_conn::Error::SendRequestFailed => ERRNO_IPC_CONN_ERROR,
        ipc_conn::Error::ReadMsgParcelFailed | ipc_conn::Error::WriteMsgParcelFailed => {
            ERRNO_IPC_RW_ERROR
        }
        ipc_conn::Error::InvalidCloudStatus => ERRNO_CLOUD_INVALID_STATUS,
        ipc_conn::Error::InvalidSpaceStatus => ERRNO_NO_SPACE_FOR_ASSET,
        ipc_conn::Error::UnlockFailed => ERRNO_UNLOCKED,
        ipc_conn::Error::DownloadFailed => ERRNO_ASSET_DOWNLOAD_FAILURE,
        ipc_conn::Error::UploadFailed => ERRNO_ASSET_UPLOAD_FAILURE,
        ipc_conn::Error::InvalidFieldType => ERRNO_INVALID_INPUT_TYPE,
        ipc_conn::Error::NetworkError => ERRNO_NETWORK_ERROR,
        ipc_conn::Error::CloudDisabled => ERRNO_CLOUD_DISABLE,
        ipc_conn::Error::LockedByOthers => ERRNO_LOCKED_BY_OTHERS,
        ipc_conn::Error::RecordLimitExceeded => ERRNO_RECORD_LIMIT_EXCEEDED,
        ipc_conn::Error::NoSpaceForAsset => ERRNO_NO_SPACE_FOR_ASSET,
        ipc_conn::Error::UnknownError => ERRNO_UNKNOWN,
        ipc_conn::Error::Success => ERRNO_SUCCESS,
    }
}

pub(crate) fn map_single_sync_err(e: &SyncError) -> c_int {
    match e {
        SyncError::Unknown => ERRNO_UNKNOWN,
        SyncError::NoSuchTableInDb => ERRNO_NO_SUCH_TABLE_IN_DB,
        SyncError::NotAnAsset => ERRNO_INVALID_INPUT_TYPE,
        SyncError::AssetDownloadFailure => ERRNO_ASSET_DOWNLOAD_FAILURE,
        SyncError::AssetUploadFailure => ERRNO_ASSET_UPLOAD_FAILURE,
        SyncError::SessionUnlocked => ERRNO_UNLOCKED,
        SyncError::Unsupported => ERRNO_UNSUPPORTED,
        SyncError::IPCError(ipc_e) => map_ipc_err(ipc_e),
        // Not deal here to involve vec in every branch
        SyncError::IPCErrors(_) => ERRNO_IPC_ERRORS,
        SyncError::IO(io_e) => match io_e.kind() {
            ErrorKind::ConnectionRefused
            | ErrorKind::ConnectionReset
            | ErrorKind::ConnectionAborted
            | ErrorKind::NotConnected
            | ErrorKind::AddrInUse
            | ErrorKind::AddrNotAvailable
            | ErrorKind::BrokenPipe
            | ErrorKind::UnexpectedEof => ERRNO_NETWORK_ERROR,
            ErrorKind::OutOfMemory => ERRNO_NO_SPACE_FOR_ASSET,
            _ => ERRNO_OTHER_IO_ERROR,
        },
    }
}

/// Create an CloudAssetLoader instance by bundle name and database pointer. This function only borrows
/// database, so this pointer won't be taken by CloudAssetLoader. Users should free it if the database
/// is no longer in need.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetLoaderNew(
    user_id: c_int,
    bundle_name: *const c_uchar,
    name_len: c_uint,
    db: *const OhCloudExtDatabase,
) -> *mut OhCloudExtCloudAssetLoader {
    if bundle_name.is_null() || db.is_null() {
        return null_mut();
    }

    let bundle_name_bytes = &*slice_from_raw_parts(bundle_name, name_len as usize);
    let bundle_name = std::str::from_utf8_unchecked(bundle_name_bytes);
    let db = match OhCloudExtDatabase::get_inner_ref(db, SafetyCheckId::Database) {
        None => return null_mut(),
        Some(v) => v,
    };

    match asset_loader::CloudAssetLoader::new(user_id, bundle_name, db) {
        Ok(loader) => {
            OhCloudExtCloudAssetLoader::new(loader, SafetyCheckId::CloudAssetLoader).into_ptr()
        }
        Err(_) => null_mut(),
    }
}

/// Information needed when upload or download assets through CloudAssetLoader.
#[repr(C)]
pub struct OhCloudExtUpDownloadInfo {
    table_name: *const c_uchar,
    table_name_len: c_uint,
    gid: *const c_uchar,
    gid_len: c_uint,
    prefix: *const c_uchar,
    prefix_len: c_uint,
}

/// Upload assets, with table name, gid, and prefix. This function only mutably borrows
/// assets, so this pointer won't be taken by CloudAssetLoader. Users should free it if the hashmap of
/// assets is no longer in need.
///
/// Assets should be in type HashMap<String, Value>.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetLoaderUpload(
    loader: *mut OhCloudExtCloudAssetLoader,
    info: *const OhCloudExtUpDownloadInfo,
    assets: *mut OhCloudExtVector,
) -> c_int {
    if loader.is_null() || info.is_null() || assets.is_null() {
        return ERRNO_NULLPTR;
    }
    let info = &*info;
    let loader =
        match OhCloudExtCloudAssetLoader::get_inner_mut(loader, SafetyCheckId::CloudAssetLoader) {
            None => return ERRNO_WRONG_TYPE,
            Some(v) => v,
        };
    let table_name_bytes = &*slice_from_raw_parts(info.table_name, info.table_name_len as usize);
    let table_name = std::str::from_utf8_unchecked(table_name_bytes);

    let gid_bytes = &*slice_from_raw_parts(info.gid, info.gid_len as usize);
    let gid = std::str::from_utf8_unchecked(gid_bytes);

    let prefix_bytes = &*slice_from_raw_parts(info.prefix, info.prefix_len as usize);
    let prefix = std::str::from_utf8_unchecked(prefix_bytes);

    let assets = match OhCloudExtVector::get_inner_ref(assets, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let assets = match assets {
        VectorCffi::CloudAsset(re) => re,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };
    match loader.upload(table_name, gid, prefix, assets) {
        Ok(_) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Download assets, with table name, gid, and prefix. This function only mutably borrows
/// assets, so this pointer won't be taken by CloudAssetLoader. Users should free it if the hashmap of
/// assets is no longer in need.
///
/// Assets should be in type HashMap<String, Value>.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetLoaderDownload(
    loader: *mut OhCloudExtCloudAssetLoader,
    info: *const OhCloudExtUpDownloadInfo,
    assets: *mut OhCloudExtVector,
) -> c_int {
    if loader.is_null() || info.is_null() || assets.is_null() {
        return ERRNO_NULLPTR;
    }
    let info = &*info;
    let loader =
        match OhCloudExtCloudAssetLoader::get_inner_mut(loader, SafetyCheckId::CloudAssetLoader) {
            None => return ERRNO_WRONG_TYPE,
            Some(v) => v,
        };
    let table_name_bytes = &*slice_from_raw_parts(info.table_name, info.table_name_len as usize);
    let table_name = std::str::from_utf8_unchecked(table_name_bytes);

    let gid_bytes = &*slice_from_raw_parts(info.gid, info.gid_len as usize);
    let gid = std::str::from_utf8_unchecked(gid_bytes);

    let prefix_bytes = &*slice_from_raw_parts(info.prefix, info.prefix_len as usize);
    let prefix = std::str::from_utf8_unchecked(prefix_bytes);

    let assets = match OhCloudExtVector::get_inner_ref(assets, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let assets = match assets {
        VectorCffi::CloudAsset(re) => re,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };

    match loader.download(table_name, gid, prefix, assets) {
        Ok(_) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Remove local asset. This function will use file system and remove a local file. This function
/// only mutably borrows asset, so this pointer won't be taken by CloudAssetLoader. Users should free
/// it if the asset is no longer in need.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetLoaderRemoveLocalAssets(
    asset: *const OhCloudExtCloudAsset,
) -> c_int {
    if asset.is_null() {
        return ERRNO_NULLPTR;
    }
    let asset = match OhCloudExtCloudAsset::get_inner_ref(asset, SafetyCheckId::CloudAsset) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match asset_loader::CloudAssetLoader::remove_local_assets(asset) {
        Ok(()) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Free an CloudAssetLoader pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudAssetLoaderFree(src: *mut OhCloudExtCloudAssetLoader) {
    let _ = OhCloudExtCloudAssetLoader::from_ptr(src, SafetyCheckId::CloudAssetLoader);
}

/// Initialize a CloudDatabase instance with bundle name and database. The database passed in will
/// be stored in CloudDatabase, so its management will be transferred and it should not be freed
/// by the users.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbNew(
    user_id: c_int,
    bundle_name: *const c_uchar,
    name_len: c_uint,
    database: *mut OhCloudExtDatabase,
) -> *mut OhCloudExtCloudDatabase {
    if database.is_null() || bundle_name.is_null() {
        return null_mut();
    }
    let database = match OhCloudExtDatabase::get_inner(database, SafetyCheckId::Database) {
        None => return null_mut(),
        Some(v) => v,
    };

    let bundle_name_bytes = &*slice_from_raw_parts(bundle_name, name_len as usize);
    let bundle_name = std::str::from_utf8_unchecked(bundle_name_bytes);

    if let Ok(db) = cloud_db::CloudDatabase::new(user_id, bundle_name, database) {
        let ptr = OhCloudExtCloudDatabase::new(db, SafetyCheckId::CloudDatabase).into_ptr();
        return ptr;
    }
    null_mut()
}

/// Sql that will passed to CloudDatabase and executed.
#[repr(C)]
pub struct OhCloudExtSql {
    table: *const c_uchar,
    table_len: c_uint,
    sql: *const c_uchar,
    sql_len: c_uint,
}

/// Execute sql on a CloudDatabase.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbExecuteSql(
    cdb: *mut OhCloudExtCloudDatabase,
    sql: *const OhCloudExtSql,
    extend: *mut OhCloudExtHashMap,
) -> c_int {
    if cdb.is_null() || sql.is_null() || extend.is_null() {
        return ERRNO_NULLPTR;
    }
    let sql = &*sql;
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let table_bytes = &*slice_from_raw_parts(sql.table, sql.table_len as usize);
    let table = std::str::from_utf8_unchecked(table_bytes);

    let sql_bytes = &*slice_from_raw_parts(sql.sql, sql.sql_len as usize);
    let sql = std::str::from_utf8_unchecked(sql_bytes);

    let extend = match OhCloudExtHashMap::get_inner_mut(extend, SafetyCheckId::HashMap) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let extend = &mut (*match extend {
        HashMapCffi::Value(re) => re,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    });
    match cdb.execute(table, sql, extend) {
        Ok(()) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Insert records into a CloudDatabase, with table name, and two Vector of HashMap<String, Value>.
/// Those two vectors passed in are only borrowed, and they should be freed by `VectorFree` if they
/// are no longer in use.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbBatchInsert(
    cdb: *mut OhCloudExtCloudDatabase,
    table: *const c_uchar,
    table_len: c_uint,
    value: *const OhCloudExtVector,
    extend: *mut OhCloudExtVector,
) -> c_int {
    if cdb.is_null() || table.is_null() || value.is_null() || extend.is_null() {
        return ERRNO_NULLPTR;
    }
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let table_bytes = &*slice_from_raw_parts(table, table_len as usize);
    let table = std::str::from_utf8_unchecked(table_bytes);

    let value = match OhCloudExtVector::get_inner_ref(value, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let value_vec = match value {
        VectorCffi::HashMapValue(val) => val,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };
    let extend = match OhCloudExtVector::get_inner_mut(extend, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let extend_vec = match extend {
        VectorCffi::HashMapValue(val) => val,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };

    match cdb.batch_insert(table, value_vec, extend_vec) {
        Ok(_) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Update records in a CloudDatabase, with table name, and two Vector of HashMap<String, Value>.
/// Those two vectors passed in are only borrowed, and they should be freed by `VectorFree` if they
/// are no longer in use.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbBatchUpdate(
    cdb: *mut OhCloudExtCloudDatabase,
    table: *const c_uchar,
    table_len: c_uint,
    value: *const OhCloudExtVector,
    extend: *mut OhCloudExtVector,
) -> c_int {
    if cdb.is_null() || table.is_null() || value.is_null() || extend.is_null() {
        return ERRNO_NULLPTR;
    }
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let table_bytes = &*slice_from_raw_parts(table, table_len as usize);
    let table = std::str::from_utf8_unchecked(table_bytes);

    let value = match OhCloudExtVector::get_inner_ref(value, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let value_vec = match value {
        VectorCffi::HashMapValue(val) => val,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };
    let extend = match OhCloudExtVector::get_inner_mut(extend, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let extend_vec = match extend {
        VectorCffi::HashMapValue(val) => val,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };

    match cdb.batch_update(table, value_vec, extend_vec) {
        Ok(_) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Delete records from a CloudDatabase, with table name, and a Vector of HashMap<String, Value>.
/// The vector passed in are only borrowed, and it should be freed by `VectorFree` if it
/// is no longer in use.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbBatchDelete(
    cdb: *mut OhCloudExtCloudDatabase,
    table: *const c_uchar,
    table_len: c_uint,
    extend: *mut OhCloudExtVector,
) -> c_int {
    if cdb.is_null() || table.is_null() || extend.is_null() {
        return ERRNO_NULLPTR;
    }
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let table_bytes = &*slice_from_raw_parts(table, table_len as usize);
    let table = std::str::from_utf8_unchecked(table_bytes);

    let extend = match OhCloudExtVector::get_inner_mut(extend, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let extend_vec = match extend {
        VectorCffi::HashMapValue(val) => val,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };

    match cdb.batch_delete(table, extend_vec) {
        Ok(_) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Query info that will passed to CloudDatabase.
#[repr(C)]
pub struct OhCloudExtQueryInfo {
    table: *const c_uchar,
    table_len: c_uint,
    cursor: *const c_uchar,
    cursor_len: c_uint,
}

/// Query records from a CloudDatabase, with table name and cursor. Return a CloudData pointer, which
/// should be freed by `CloudDataFree` if it's no longer in use.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbBatchQuery(
    cdb: *mut OhCloudExtCloudDatabase,
    info: *const OhCloudExtQueryInfo,
    out: *mut *const OhCloudExtCloudDbData,
) -> c_int {
    if cdb.is_null() || info.is_null() || out.is_null() {
        return -1;
    }
    let info = &*info;
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let table_bytes = &*slice_from_raw_parts(info.table, info.table_len as usize);
    let table = std::str::from_utf8_unchecked(table_bytes);

    let cursor_bytes = &*slice_from_raw_parts(info.cursor, info.cursor_len as usize);
    let cursor = std::str::from_utf8_unchecked(cursor_bytes);

    match cdb.batch_query(table, cursor) {
        Ok(cloud_data) => {
            *out = OhCloudExtCloudDbData::new(cloud_data, SafetyCheckId::CloudDbData).into_ptr();
            ERRNO_SUCCESS
        }
        Err(e) => map_single_sync_err(&e),
    }
}

/// Lock a CloudDatabase. Return expire time.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbLock(
    cdb: *mut OhCloudExtCloudDatabase,
    expire: *mut c_int,
) -> c_int {
    if cdb.is_null() || expire.is_null() {
        return ERRNO_NULLPTR;
    }
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match cdb.lock() {
        Ok(time) => {
            *expire = time;
            ERRNO_SUCCESS
        }
        Err(e) => map_single_sync_err(&e),
    }
}

/// Unlock a CloudDatabase.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbUnlock(cdb: *mut OhCloudExtCloudDatabase) -> c_int {
    if cdb.is_null() {
        return ERRNO_NULLPTR;
    }
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match cdb.unlock() {
        Ok(()) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Heartbeat function of a CloudDatabase. Extend the current locking session.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbHeartbeat(cdb: *mut OhCloudExtCloudDatabase) -> c_int {
    if cdb.is_null() {
        return ERRNO_NULLPTR;
    }
    let cdb = match OhCloudExtCloudDatabase::get_inner_mut(cdb, SafetyCheckId::CloudDatabase) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match cdb.heartbeat() {
        Ok(()) => ERRNO_SUCCESS,
        Err(e) => map_single_sync_err(&e),
    }
}

/// Free a CloudDatabase pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudDbFree(cdb: *mut OhCloudExtCloudDatabase) {
    let _ = OhCloudExtCloudDatabase::from_ptr(cdb, SafetyCheckId::CloudDatabase);
}

/// Create a CloudSync instance by user id.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudSyncNew(user_id: c_int) -> *mut OhCloudExtCloudSync {
    if let Ok(service) = cloud_service::CloudSync::new(user_id) {
        return OhCloudExtCloudSync::new(service, SafetyCheckId::CloudSync).into_ptr();
    }
    null_mut()
}

/// Get service info from a CloudSync pointer. Return a CloudInfo pointer, which
/// should be freed by `CloudInfoFree` if it's no longer in use.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudSyncGetServiceInfo(
    server: *mut OhCloudExtCloudSync,
    info: *mut *const OhCloudExtCloudInfo,
) -> c_int {
    if server.is_null() || info.is_null() {
        return ERRNO_NULLPTR;
    }
    let cloud_server = match OhCloudExtCloudSync::get_inner_mut(server, SafetyCheckId::CloudSync) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match cloud_server.get_service_info() {
        Ok(cloud_info) => {
            *info = OhCloudExtCloudInfo::new(cloud_info, SafetyCheckId::CloudInfo).into_ptr();
            ERRNO_SUCCESS
        }
        Err(e) => map_single_sync_err(&e),
    }
}

/// Get app schema from a CloudSync pointer, with a bundle name. Return a SchemaMeta pointer, which
/// should be freed by `SchemaMetaFree` if it's no longer in use.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudSyncGetAppSchema(
    server: *mut OhCloudExtCloudSync,
    bundle_name: *const c_uchar,
    bundle_name_len: c_uint,
    schema: *mut *const OhCloudExtSchemaMeta,
) -> c_int {
    if server.is_null() || bundle_name.is_null() || schema.is_null() {
        return ERRNO_NULLPTR;
    }
    let cloud_server = match OhCloudExtCloudSync::get_inner_mut(server, SafetyCheckId::CloudSync) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let bundle_name_bytes = &*slice_from_raw_parts(bundle_name, bundle_name_len as usize);
    let bundle_name = std::str::from_utf8_unchecked(bundle_name_bytes);

    match cloud_server.get_app_schema(bundle_name) {
        Ok(schema_meta) => {
            *schema = OhCloudExtSchemaMeta::new(schema_meta, SafetyCheckId::SchemaMeta).into_ptr();
            ERRNO_SUCCESS
        }
        Err(e) => map_single_sync_err(&e),
    }
}

/// Pass a batch of subscription orders to a CloudSync pointer, with target database information
/// and expected expire time. Database information should be in type HashMap<String, Vec<Database>>.
/// Return a Hashmap of subscription relations, in the form HashMap<String, RelationSet>, and Vector
/// of errors in c_int, if failure happens. The Hashmap and Vector returned should be freed by
/// `HashMapFree` and `VectorFree` respectively, if not in use anymore.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudSyncSubscribe(
    server: *mut OhCloudExtCloudSync,
    dbs: *const OhCloudExtHashMap,
    expire: c_longlong,
    relations: *mut *const OhCloudExtHashMap,
    err: *mut *const OhCloudExtVector,
) -> c_int {
    if server.is_null() || dbs.is_null() || relations.is_null() || err.is_null() {
        return ERRNO_NULLPTR;
    }

    let cloud_server = match OhCloudExtCloudSync::get_inner_mut(server, SafetyCheckId::CloudSync) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let dbs = match OhCloudExtHashMap::get_inner_ref(dbs, SafetyCheckId::HashMap) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let dbs = match dbs {
        HashMapCffi::VecDatabase(databases) => databases,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };

    match cloud_server.subscribe(dbs, expire) {
        Ok(ret) => {
            let result = HashMapCffi::RelationSet(ret);
            *relations = OhCloudExtHashMap::new(result, SafetyCheckId::HashMap).into_ptr();
            *err = null_mut();
            ERRNO_SUCCESS
        }
        Err(e) => {
            let errno = map_single_sync_err(&e);
            if errno == ERRNO_IPC_ERRORS {
                if let SyncError::IPCErrors(vec) = e {
                    let ret = VectorCffi::I32(vec.iter().map(map_ipc_err).collect());
                    *err = OhCloudExtVector::new(ret, SafetyCheckId::Vector).into_ptr();
                }
            }
            errno
        }
    }
}

/// Pass a batch of unsubscription orders to a CloudSync pointer, with target relations.
/// Relations should be in type HashMap<String, Vec<String>>, with bundle name as keys, vector of
/// subscription ids as value. Return a Vector of errors in c_int, if failure happens. The Vector
/// returned should be freed by `VectorFree` respectively, if not in use anymore.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudSyncUnsubscribe(
    server: *mut OhCloudExtCloudSync,
    relations: *const OhCloudExtHashMap,
    err: *mut *const OhCloudExtVector,
) -> c_int {
    if server.is_null() || relations.is_null() || err.is_null() {
        return ERRNO_NULLPTR;
    }

    let cloud_server = match OhCloudExtCloudSync::get_inner_mut(server, SafetyCheckId::CloudSync) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    let relations = match OhCloudExtHashMap::get_inner_ref(relations, SafetyCheckId::HashMap) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let relations = match relations {
        HashMapCffi::VecString(res) => res,
        _ => return ERRNO_INVALID_INPUT_TYPE,
    };

    match cloud_server.unsubscribe(relations) {
        Ok(()) => ERRNO_SUCCESS,
        Err(e) => {
            let errno = map_single_sync_err(&e);
            if errno == ERRNO_IPC_ERRORS {
                if let SyncError::IPCErrors(vec) = e {
                    let ret = VectorCffi::I32(vec.iter().map(map_ipc_err).collect());
                    *err = OhCloudExtVector::new(ret, SafetyCheckId::Vector).into_ptr();
                }
            }
            errno
        }
    }
}

/// Free a CloudSync pointer.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtCloudSyncFree(server: *mut OhCloudExtCloudSync) {
    let _ = OhCloudExtCloudSync::from_ptr(server, SafetyCheckId::CloudSync);
}
