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

use crate::c_adapter::*;
use crate::ipc_conn;
use crate::service_impl::{cloud_service, types};
use std::collections::HashMap;
use std::ffi::{c_int, c_uchar, c_uint, c_void};
use std::ptr::null_mut;

pub type OhCloudExtVector = SafeCffiWrapper<VectorCffi>;
pub type OhCloudExtHashMap = SafeCffiWrapper<HashMapCffi>;

/// Value type enum in C. Used to represent returning type value, so C side can cast its pointer.
#[repr(C)]
#[allow(non_camel_case_types, clippy::upper_case_acronyms)]
#[derive(PartialEq, Debug)]
pub enum OhCloudExtRustType {
    NULL = 0,
    U32,
    I32,
    STRING,
    VALUE,
    VALUE_BUCKET,
    DATABASE,
    TABLE,
    FIELD,
    RELATION,
    RELATION_SET,
    CLOUD_ASSET,
    APP_INFO,
    VEC_U32,
    VEC_STRING,
    VEC_DATABASE,
    HASHMAP_VALUE,
}

/// Vector enum in C side to adapt Vector generic types.
pub enum VectorCffi {
    I32(Vec<i32>),
    U32(Vec<u32>),
    String(Vec<String>),
    Value(Vec<types::Value>),
    ValueBucket(Vec<ipc_conn::ValueBucket>),
    Database(Vec<types::Database>),
    Table(Vec<types::Table>),
    Field(Vec<types::Field>),
    RelationSet(Vec<cloud_service::RelationSet>),
    CloudAsset(Vec<ipc_conn::CloudAsset>),
    AppInfo(Vec<cloud_service::AppInfo>),
    VecU32(Vec<Vec<u32>>),
    VecString(Vec<Vec<String>>),
    VecDatabase(Vec<Vec<types::Database>>),
    HashMapValue(Vec<HashMap<String, types::Value>>),
}

/// Hashmap enum in C side to adapt Vector generic types. String as the key, enum type only
/// marks value type.
pub enum HashMapCffi {
    U32(HashMap<String, u32>),
    String(HashMap<String, String>),
    Value(HashMap<String, types::Value>),
    Table(HashMap<String, types::Table>),
    RelationSet(HashMap<String, cloud_service::RelationSet>),
    AppInfo(HashMap<String, cloud_service::AppInfo>),
    VecU32(HashMap<String, Vec<u32>>),
    VecString(HashMap<String, Vec<String>>),
    VecDatabase(HashMap<String, Vec<types::Database>>),
}

/// Create a Vector enum according to ValueType.
///
/// # Safety
/// For other functions relating to Vector usage, the pointer of Vector should be valid through
/// the lifetime of these function calls. Also, when using String values, users should guarantee
/// that String doesn't contain non-UTF8 literals, and the length passed in is valid.
///
/// These two requirements need to be fulfilled to avoid memory issues and undefined behaviors.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtVectorNew(typ: OhCloudExtRustType) -> *mut OhCloudExtVector {
    let vec = match typ {
        OhCloudExtRustType::I32 => VectorCffi::I32(vec![]),
        OhCloudExtRustType::U32 => VectorCffi::U32(vec![]),
        OhCloudExtRustType::STRING => VectorCffi::String(vec![]),
        OhCloudExtRustType::VALUE => VectorCffi::Value(vec![]),
        OhCloudExtRustType::VALUE_BUCKET => VectorCffi::ValueBucket(vec![]),
        OhCloudExtRustType::DATABASE => VectorCffi::Database(vec![]),
        OhCloudExtRustType::TABLE => VectorCffi::Table(vec![]),
        OhCloudExtRustType::FIELD => VectorCffi::Field(vec![]),
        OhCloudExtRustType::RELATION_SET => VectorCffi::RelationSet(vec![]),
        OhCloudExtRustType::CLOUD_ASSET => VectorCffi::CloudAsset(vec![]),
        OhCloudExtRustType::APP_INFO => VectorCffi::AppInfo(vec![]),
        OhCloudExtRustType::VEC_U32 => VectorCffi::VecU32(vec![]),
        OhCloudExtRustType::VEC_STRING => VectorCffi::VecString(vec![]),
        OhCloudExtRustType::VEC_DATABASE => VectorCffi::VecDatabase(vec![]),
        OhCloudExtRustType::HASHMAP_VALUE => VectorCffi::HashMapValue(vec![]),
        // Not supported
        _ => return null_mut(),
    };
    OhCloudExtVector::new(vec, SafetyCheckId::Vector).into_ptr()
}

/// Get ValueType of the Vector pointer.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtVectorGetValueTyp(
    vector: *const OhCloudExtVector,
) -> OhCloudExtRustType {
    let vector = match OhCloudExtVector::get_inner_ref(vector, SafetyCheckId::Vector) {
        None => return OhCloudExtRustType::NULL,
        Some(v) => v,
    };
    match vector {
        VectorCffi::I32(_) => OhCloudExtRustType::I32,
        VectorCffi::U32(_) => OhCloudExtRustType::U32,
        VectorCffi::String(_) => OhCloudExtRustType::STRING,
        VectorCffi::Value(_) => OhCloudExtRustType::VALUE,
        VectorCffi::ValueBucket(_) => OhCloudExtRustType::VALUE_BUCKET,
        VectorCffi::Database(_) => OhCloudExtRustType::DATABASE,
        VectorCffi::Table(_) => OhCloudExtRustType::TABLE,
        VectorCffi::Field(_) => OhCloudExtRustType::FIELD,
        VectorCffi::RelationSet(_) => OhCloudExtRustType::RELATION_SET,
        VectorCffi::CloudAsset(_) => OhCloudExtRustType::CLOUD_ASSET,
        VectorCffi::AppInfo(_) => OhCloudExtRustType::APP_INFO,
        VectorCffi::VecU32(_) => OhCloudExtRustType::VEC_U32,
        VectorCffi::VecString(_) => OhCloudExtRustType::VEC_STRING,
        VectorCffi::VecDatabase(_) => OhCloudExtRustType::VEC_DATABASE,
        VectorCffi::HashMapValue(_) => OhCloudExtRustType::HASHMAP_VALUE,
    }
}

/// Push value into the Vector pointer. If the value pushed is allocated in the Rust side before,
/// pushing them means to transfer their management to the Vector, so no more free is needed.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen. Besides, value pointer should also be in the same type as the type used in
/// initialization of the Vector.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtVectorPush(
    vector: *mut OhCloudExtVector,
    value: *mut c_void,
    value_len: c_uint,
) -> c_int {
    if vector.is_null() || value.is_null() {
        return ERRNO_NULLPTR;
    }

    let vector = match OhCloudExtVector::get_inner_mut(vector, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    match vector {
        VectorCffi::I32(vec) => {
            let ptr = value as *mut i32;
            vec.push(*ptr);
        }
        VectorCffi::U32(vec) => {
            let ptr = value as *mut u32;
            vec.push(*ptr);
        }
        VectorCffi::String(vec) => {
            let str = char_ptr_to_string(value as *const c_uchar, value_len);
            vec.push(str);
        }
        VectorCffi::Value(vec) => {
            let val = match OhCloudExtValue::get_inner(value as *mut _, SafetyCheckId::Value) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            vec.push(val);
        }
        VectorCffi::ValueBucket(vec) => {
            let vb =
                match OhCloudExtValueBucket::get_inner(value as *mut _, SafetyCheckId::ValueBucket)
                {
                    None => return ERRNO_WRONG_TYPE,
                    Some(v) => v,
                };
            vec.push(vb);
        }
        VectorCffi::Database(vec) => {
            let db = match OhCloudExtDatabase::get_inner(value as *mut _, SafetyCheckId::Database) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            vec.push(db);
        }
        VectorCffi::Table(vec) => {
            let val = match OhCloudExtTable::get_inner(value as *mut _, SafetyCheckId::Table) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            vec.push(val);
        }
        VectorCffi::Field(vec) => {
            let fd = match OhCloudExtField::get_inner(value as *mut _, SafetyCheckId::Field) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            vec.push(fd);
        }
        VectorCffi::RelationSet(vec) => {
            let val =
                match OhCloudExtRelationSet::get_inner(value as *mut _, SafetyCheckId::RelationSet)
                {
                    None => return ERRNO_WRONG_TYPE,
                    Some(v) => v,
                };
            vec.push(val);
        }
        VectorCffi::CloudAsset(vec) => {
            let cloud_asset =
                match OhCloudExtCloudAsset::get_inner(value as *mut _, SafetyCheckId::CloudAsset) {
                    None => return ERRNO_WRONG_TYPE,
                    Some(v) => v,
                };
            vec.push(cloud_asset);
        }
        VectorCffi::AppInfo(vec) => {
            let val = match OhCloudExtAppInfo::get_inner(value as *mut _, SafetyCheckId::AppInfo) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            vec.push(val);
        }
        VectorCffi::VecU32(vec) => {
            let vec_struct =
                match OhCloudExtVector::get_inner(value as *mut _, SafetyCheckId::Vector) {
                    None => return ERRNO_WRONG_TYPE,
                    Some(v) => v,
                };
            match vec_struct {
                VectorCffi::U32(v) => vec.push(v),
                _ => return ERRNO_INVALID_INPUT_TYPE,
            }
        }
        VectorCffi::VecString(vec) => {
            let vec_struct =
                match OhCloudExtVector::get_inner(value as *mut _, SafetyCheckId::Vector) {
                    None => return ERRNO_WRONG_TYPE,
                    Some(v) => v,
                };
            match vec_struct {
                VectorCffi::String(v) => vec.push(v),
                _ => return ERRNO_INVALID_INPUT_TYPE,
            }
        }
        VectorCffi::VecDatabase(vec) => {
            let vec_struct =
                match OhCloudExtVector::get_inner(value as *mut _, SafetyCheckId::Vector) {
                    None => return ERRNO_WRONG_TYPE,
                    Some(v) => v,
                };
            match vec_struct {
                VectorCffi::Database(v) => vec.push(v),
                _ => return ERRNO_INVALID_INPUT_TYPE,
            }
        }
        VectorCffi::HashMapValue(vec) => {
            let map = match OhCloudExtHashMap::get_inner(value as *mut _, SafetyCheckId::HashMap) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            match map {
                HashMapCffi::Value(m) => vec.push(m),
                _ => return ERRNO_INVALID_INPUT_TYPE,
            }
        }
    }
    ERRNO_SUCCESS
}

/// Get value from the Vector pointer. If returning type is not raw C type
/// pointer, these pointers should be freed by corresponding free functions.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen. Besides, value pointer should also be interpreted as same as the type
/// used in initialization of the Vector.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtVectorGet(
    vector: *const OhCloudExtVector,
    index: usize,
    value: *mut *const c_void,
    value_len: *mut c_uint,
) -> c_int {
    macro_rules! get_content {
        ($vec: ident, $index: ident, $value: ident) => {
            if $index < $vec.len() {
                *$value = (&$vec[index]) as *const _ as *const c_void;
            } else {
                return ERRNO_OUT_OF_RANGE;
            }
        };
    }

    macro_rules! get_content_clone {
        ($vec: ident, $index: ident, $value: ident, $typ: ident, $id: ident) => {
            if $index < $vec.len() {
                *$value =
                    $typ::new($vec[index].clone(), SafetyCheckId::$id).into_ptr() as *const c_void;
            } else {
                return ERRNO_OUT_OF_RANGE;
            }
        };
    }

    if vector.is_null() || value.is_null() || value_len.is_null() {
        return ERRNO_NULLPTR;
    }

    let vector = match OhCloudExtVector::get_inner_ref(vector, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };

    match vector {
        VectorCffi::I32(vec) => get_content!(vec, index, value),
        VectorCffi::U32(vec) => get_content!(vec, index, value),
        VectorCffi::String(vec) => {
            if index < vec.len() {
                *value = vec[index].as_ptr() as *const c_void;
                *value_len = vec[index].len() as c_uint;
            } else {
                return ERRNO_OUT_OF_RANGE;
            }
        }
        VectorCffi::Value(vec) => get_content_clone!(vec, index, value, OhCloudExtValue, Value),
        VectorCffi::ValueBucket(vec) => {
            get_content_clone!(vec, index, value, OhCloudExtValueBucket, ValueBucket)
        }
        VectorCffi::Database(vec) => {
            get_content_clone!(vec, index, value, OhCloudExtDatabase, Database)
        }
        VectorCffi::Table(vec) => get_content_clone!(vec, index, value, OhCloudExtTable, Table),
        VectorCffi::Field(vec) => get_content_clone!(vec, index, value, OhCloudExtField, Field),
        VectorCffi::RelationSet(vec) => {
            get_content_clone!(vec, index, value, OhCloudExtRelationSet, RelationSet)
        }
        VectorCffi::CloudAsset(vec) => {
            get_content_clone!(vec, index, value, OhCloudExtCloudAsset, CloudAsset)
        }
        VectorCffi::AppInfo(vec) => {
            get_content_clone!(vec, index, value, OhCloudExtAppInfo, AppInfo)
        }
        VectorCffi::VecU32(vec) => {
            if index < vec.len() {
                let src = &vec[index];
                *value = OhCloudExtVector::new(VectorCffi::U32(src.clone()), SafetyCheckId::Vector)
                    .into_ptr() as *const c_void;
            } else {
                return ERRNO_OUT_OF_RANGE;
            }
        }
        VectorCffi::VecString(vec) => {
            if index < vec.len() {
                let src = &vec[index];
                *value =
                    OhCloudExtVector::new(VectorCffi::String(src.clone()), SafetyCheckId::Vector)
                        .into_ptr() as *const c_void;
            } else {
                return ERRNO_OUT_OF_RANGE;
            }
        }
        VectorCffi::VecDatabase(vec) => {
            if index < vec.len() {
                let src = &vec[index];
                *value =
                    OhCloudExtVector::new(VectorCffi::Database(src.clone()), SafetyCheckId::Vector)
                        .into_ptr() as *const c_void;
            } else {
                return ERRNO_OUT_OF_RANGE;
            }
        }
        VectorCffi::HashMapValue(vec) => {
            if index < vec.len() {
                let src = &vec[index];
                let mut inner = HashMap::new();
                for (key, value) in src {
                    inner.insert(key.clone(), value.clone());
                }
                *value = OhCloudExtHashMap::new(HashMapCffi::Value(inner), SafetyCheckId::HashMap)
                    .into_ptr() as *const c_void;
            } else {
                return ERRNO_OUT_OF_RANGE;
            }
        }
    }
    ERRNO_SUCCESS
}

/// Get length of a Vector pointer.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtVectorGetLength(
    vector: *const OhCloudExtVector,
    len: *mut c_uint,
) -> c_int {
    if vector.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }
    let vector = match OhCloudExtVector::get_inner_ref(vector, SafetyCheckId::Vector) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match vector {
        VectorCffi::I32(vec) => *len = vec.len() as c_uint,
        VectorCffi::U32(vec) => *len = vec.len() as c_uint,
        VectorCffi::String(vec) => *len = vec.len() as c_uint,
        VectorCffi::Value(vec) => *len = vec.len() as c_uint,
        VectorCffi::ValueBucket(vec) => *len = vec.len() as c_uint,
        VectorCffi::Database(vec) => *len = vec.len() as c_uint,
        VectorCffi::Table(vec) => *len = vec.len() as c_uint,
        VectorCffi::Field(vec) => *len = vec.len() as c_uint,
        VectorCffi::RelationSet(vec) => *len = vec.len() as c_uint,
        VectorCffi::CloudAsset(vec) => *len = vec.len() as c_uint,
        VectorCffi::AppInfo(vec) => *len = vec.len() as c_uint,
        VectorCffi::VecU32(vec) => *len = vec.len() as c_uint,
        VectorCffi::VecString(vec) => *len = vec.len() as c_uint,
        VectorCffi::VecDatabase(vec) => *len = vec.len() as c_uint,
        VectorCffi::HashMapValue(vec) => *len = vec.len() as c_uint,
    }
    ERRNO_SUCCESS
}

/// Free a Vector pointer.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtVectorFree(vector: *mut OhCloudExtVector) {
    let _ = OhCloudExtVector::from_ptr(vector, SafetyCheckId::Vector);
}

/// Initialize a HashMap enum by its ValueType. The key type is fixed to be String.
///
/// # Safety
/// For other functions relating to HashMap usage, the pointer of HashMap should be valid through
/// the lifetime of these function calls. Also, when using String values, users should guarantee
/// that String doesn't contain non-UTF8 literals, and the length passed in is valid.
///
/// These two requirements need to be fulfilled to avoid memory issues and undefined behaviors.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapNew(
    value_type: OhCloudExtRustType,
) -> *mut OhCloudExtHashMap {
    let map = match value_type {
        OhCloudExtRustType::U32 => HashMapCffi::U32(HashMap::default()),
        OhCloudExtRustType::STRING => HashMapCffi::String(HashMap::default()),
        OhCloudExtRustType::VALUE => HashMapCffi::Value(HashMap::default()),
        OhCloudExtRustType::TABLE => HashMapCffi::Table(HashMap::default()),
        OhCloudExtRustType::RELATION => HashMapCffi::RelationSet(HashMap::default()),
        OhCloudExtRustType::APP_INFO => HashMapCffi::AppInfo(HashMap::default()),
        OhCloudExtRustType::VEC_U32 => HashMapCffi::VecU32(HashMap::default()),
        OhCloudExtRustType::VEC_STRING => HashMapCffi::VecString(HashMap::default()),
        OhCloudExtRustType::VEC_DATABASE => HashMapCffi::VecDatabase(HashMap::default()),
        // Not supported
        _ => return null_mut(),
    };
    OhCloudExtHashMap::new(map, SafetyCheckId::HashMap).into_ptr()
}

/// Get key type of a Hashmap pointer.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapGetKeyTyp(
    hash_map: *const OhCloudExtHashMap,
) -> OhCloudExtRustType {
    if hash_map.is_null() {
        OhCloudExtRustType::NULL
    } else {
        OhCloudExtRustType::STRING
    }
}

/// Get value type of a Hashmap pointer.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapGetValueTyp(
    hash_map: *const OhCloudExtHashMap,
) -> OhCloudExtRustType {
    if hash_map.is_null() {
        return OhCloudExtRustType::NULL;
    }
    let hash_map = match OhCloudExtHashMap::get_inner_ref(hash_map, SafetyCheckId::HashMap) {
        None => return OhCloudExtRustType::NULL,
        Some(v) => v,
    };
    match hash_map {
        HashMapCffi::U32(_) => OhCloudExtRustType::U32,
        HashMapCffi::String(_) => OhCloudExtRustType::STRING,
        HashMapCffi::Value(_) => OhCloudExtRustType::VALUE,
        HashMapCffi::Table(_) => OhCloudExtRustType::TABLE,
        HashMapCffi::RelationSet(_) => OhCloudExtRustType::RELATION,
        HashMapCffi::AppInfo(_) => OhCloudExtRustType::APP_INFO,
        HashMapCffi::VecU32(_) => OhCloudExtRustType::VEC_U32,
        HashMapCffi::VecString(_) => OhCloudExtRustType::VEC_STRING,
        HashMapCffi::VecDatabase(_) => OhCloudExtRustType::VEC_DATABASE,
    }
}

/// Get length of a Hashmap pointer.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapGetLength(
    hash_map: *const OhCloudExtHashMap,
    len: *mut c_uint,
) -> c_int {
    if hash_map.is_null() || len.is_null() {
        return ERRNO_NULLPTR;
    }
    let hash_map = match OhCloudExtHashMap::get_inner_ref(hash_map, SafetyCheckId::HashMap) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match hash_map {
        HashMapCffi::U32(map) => *len = map.len() as c_uint,
        HashMapCffi::String(map) => *len = map.len() as c_uint,
        HashMapCffi::Value(map) => *len = map.len() as c_uint,
        HashMapCffi::Table(map) => *len = map.len() as c_uint,
        HashMapCffi::RelationSet(map) => *len = map.len() as c_uint,
        HashMapCffi::AppInfo(map) => *len = map.len() as c_uint,
        HashMapCffi::VecU32(map) => *len = map.len() as c_uint,
        HashMapCffi::VecString(map) => *len = map.len() as c_uint,
        HashMapCffi::VecDatabase(map) => *len = map.len() as c_uint,
    }
    ERRNO_SUCCESS
}

/// Insert key, value pairs into the Hashmap pointer. If the value pushed is allocated in the Rust
/// side before, pushing them means to transfer their management to the Hashmap, so no more free
/// is needed.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen. Besides, value and key pointers should also be in the same types as the types
/// used in initialization of the HashMap.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapInsert(
    hash_map: *mut OhCloudExtHashMap,
    key: *const c_uchar,
    key_len: c_uint,
    value: *mut c_void,
    value_len: c_uint,
) -> c_int {
    if hash_map.is_null() || key.is_null() || value.is_null() {
        return ERRNO_NULLPTR;
    }
    let hash_map = match OhCloudExtHashMap::get_inner_mut(hash_map, SafetyCheckId::HashMap) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let key = char_ptr_to_string(key as *const c_uchar, key_len);
    match hash_map {
        HashMapCffi::U32(map) => {
            let ptr = value as *mut u32;
            map.insert(key, *ptr);
        }
        HashMapCffi::String(map) => {
            let value = char_ptr_to_string(value as *const c_uchar, value_len);
            map.insert(key, value);
        }
        HashMapCffi::Value(map) => {
            let val = match OhCloudExtValue::get_inner(value as *mut _, SafetyCheckId::Value) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            map.insert(key, val);
        }
        HashMapCffi::Table(map) => {
            let val = match OhCloudExtTable::get_inner(value as *mut _, SafetyCheckId::Table) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            map.insert(key, val);
        }
        HashMapCffi::RelationSet(map) => {
            let val =
                match OhCloudExtRelationSet::get_inner(value as *mut _, SafetyCheckId::RelationSet)
                {
                    None => return ERRNO_WRONG_TYPE,
                    Some(v) => v,
                };
            map.insert(key, val);
        }
        HashMapCffi::AppInfo(map) => {
            let val = match OhCloudExtAppInfo::get_inner(value as *mut _, SafetyCheckId::AppInfo) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            map.insert(key, val);
        }
        HashMapCffi::VecU32(map) => {
            let val = match OhCloudExtVector::get_inner(value as *mut _, SafetyCheckId::Vector) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            match val {
                VectorCffi::U32(vec) => map.insert(key, vec),
                _ => return ERRNO_INVALID_INPUT_TYPE,
            };
        }
        HashMapCffi::VecString(map) => {
            let val = match OhCloudExtVector::get_inner(value as *mut _, SafetyCheckId::Vector) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            match val {
                VectorCffi::String(vec) => map.insert(key, vec),
                _ => return ERRNO_INVALID_INPUT_TYPE,
            };
        }
        HashMapCffi::VecDatabase(map) => {
            let val = match OhCloudExtVector::get_inner(value as *mut _, SafetyCheckId::Vector) {
                None => return ERRNO_WRONG_TYPE,
                Some(v) => v,
            };
            match val {
                VectorCffi::Database(vec) => map.insert(key, vec),
                _ => return ERRNO_INVALID_INPUT_TYPE,
            };
        }
    }
    ERRNO_SUCCESS
}

/// Get key, value pair from the Hashmap pointer. The returning type is `Vector`, and should be
/// freed by `VectorFree`.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen. Besides, value and key pointer should also be interpreted as same types as the
/// types used in initialization of the HashMap.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapIterGetKeyValuePair(
    hash_map: *const OhCloudExtHashMap,
    key: *mut *const OhCloudExtVector,
    value: *mut *const OhCloudExtVector,
) -> c_int {
    if hash_map.is_null() || key.is_null() || value.is_null() {
        return ERRNO_NULLPTR;
    }
    let hash_map = match OhCloudExtHashMap::get_inner_ref(hash_map, SafetyCheckId::HashMap) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    match hash_map {
        HashMapCffi::U32(map) => {
            let key_vec = map.keys().cloned().collect();
            let value_vec: Vec<u32> = map.values().cloned().collect();

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value = OhCloudExtVector::new(VectorCffi::U32(value_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::String(map) => {
            let key_vec = map.keys().cloned().collect();
            let value_vec: Vec<String> = map.values().cloned().collect();

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value = OhCloudExtVector::new(VectorCffi::String(value_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::Value(map) => {
            let key_vec: Vec<String> = map.keys().cloned().collect();
            let value_vec: Vec<types::Value> = map.values().cloned().collect();

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value = OhCloudExtVector::new(VectorCffi::Value(value_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::Table(map) => {
            let key_vec: Vec<String> = map.keys().cloned().collect();
            let mut value_vec = vec![];
            for src in map.values() {
                value_vec.push(src.clone());
            }

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value = OhCloudExtVector::new(VectorCffi::Table(value_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::RelationSet(map) => {
            let key_vec: Vec<String> = map.keys().cloned().collect();
            let mut value_vec = vec![];
            for src in map.values() {
                value_vec.push(src.clone());
            }

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value =
                OhCloudExtVector::new(VectorCffi::RelationSet(value_vec), SafetyCheckId::Vector)
                    .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::AppInfo(map) => {
            let key_vec: Vec<String> = map.keys().cloned().collect();
            let mut value_vec = vec![];
            for src in map.values() {
                value_vec.push(src.clone());
            }

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value = OhCloudExtVector::new(VectorCffi::AppInfo(value_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::VecU32(map) => {
            let key_vec: Vec<String> = map.keys().cloned().collect();
            let value_vec: Vec<Vec<u32>> = map.values().cloned().collect();

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value = OhCloudExtVector::new(VectorCffi::VecU32(value_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::VecString(map) => {
            let key_vec: Vec<String> = map.keys().cloned().collect();
            let value_vec: Vec<Vec<String>> = map.values().cloned().collect();

            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value = OhCloudExtVector::new(VectorCffi::VecString(value_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
        }
        HashMapCffi::VecDatabase(map) => {
            let key_vec: Vec<String> = map.keys().cloned().collect();
            let mut value_vec = vec![];
            for src in map.values() {
                value_vec.push(src.clone());
            }
            *key = OhCloudExtVector::new(VectorCffi::String(key_vec), SafetyCheckId::Vector)
                .into_ptr() as *const OhCloudExtVector;
            *value =
                OhCloudExtVector::new(VectorCffi::VecDatabase(value_vec), SafetyCheckId::Vector)
                    .into_ptr() as *const OhCloudExtVector;
        }
    }
    ERRNO_SUCCESS
}

/// According to key, get value from the Hashmap pointer. If returning type is not raw C type
/// pointer, these pointers should be freed by corresponding free functions.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen. Besides, pointers should also be interpreted as same as the types used in
/// initialization of the Vector.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapGet(
    hash_map: *const OhCloudExtHashMap,
    key: *const c_uchar,
    key_len: c_uint,
    value: *mut *const c_void,
    value_len: *mut c_uint,
) -> c_int {
    if hash_map.is_null() || key.is_null() || value.is_null() || value_len.is_null() {
        return ERRNO_NULLPTR;
    }
    let hash_map = match OhCloudExtHashMap::get_inner_ref(hash_map, SafetyCheckId::HashMap) {
        None => return ERRNO_WRONG_TYPE,
        Some(v) => v,
    };
    let key_bytes = &*slice_from_raw_parts(key, key_len as usize);
    let key = std::str::from_utf8_unchecked(key_bytes);
    match hash_map {
        HashMapCffi::U32(map) => {
            if let Some(val) = map.get(key) {
                *value = val as *const _ as *const c_void;
            }
        }
        HashMapCffi::String(map) => {
            if let Some(val) = map.get(key) {
                *value = val.as_ptr() as *const c_void;
                *value_len = val.len() as c_uint;
            }
        }
        HashMapCffi::Value(map) => {
            if let Some(val) = map.get(key) {
                *value = OhCloudExtValue::new(val.clone(), SafetyCheckId::Value).into_ptr()
                    as *const c_void;
            }
        }
        HashMapCffi::Table(map) => {
            if let Some(val) = map.get(key) {
                *value = OhCloudExtTable::new(val.clone(), SafetyCheckId::Table).into_ptr()
                    as *const c_void;
            }
        }
        HashMapCffi::RelationSet(map) => {
            if let Some(val) = map.get(key) {
                *value = OhCloudExtRelationSet::new(val.clone(), SafetyCheckId::RelationSet)
                    .into_ptr() as *const c_void;
            }
        }
        HashMapCffi::AppInfo(map) => {
            if let Some(val) = map.get(key) {
                *value = OhCloudExtAppInfo::new(val.clone(), SafetyCheckId::AppInfo).into_ptr()
                    as *const c_void;
            }
        }
        HashMapCffi::VecU32(map) => {
            if let Some(val) = map.get(key) {
                *value = OhCloudExtVector::new(VectorCffi::U32(val.clone()), SafetyCheckId::Vector)
                    .into_ptr() as *const c_void;
            }
        }
        HashMapCffi::VecString(map) => {
            if let Some(val) = map.get(key) {
                *value =
                    OhCloudExtVector::new(VectorCffi::String(val.clone()), SafetyCheckId::Vector)
                        .into_ptr() as *const c_void;
            }
        }
        HashMapCffi::VecDatabase(map) => {
            if let Some(val) = map.get(key) {
                *value =
                    OhCloudExtVector::new(VectorCffi::Database(val.clone()), SafetyCheckId::Vector)
                        .into_ptr() as *const c_void;
            }
        }
    }
    ERRNO_SUCCESS
}

/// Free a Hashmap pointer.
///
/// # Safety
/// Pointer passed in should be valid during the whole lifetime of this function, or memory issues
/// might happen.
#[no_mangle]
pub unsafe extern "C" fn OhCloudExtHashMapFree(hash_map: *mut OhCloudExtHashMap) {
    let _ = OhCloudExtHashMap::from_ptr(hash_map, SafetyCheckId::HashMap);
}

#[cfg(test)]
mod test {
    use crate::c_adapter::basic_rust_types::*;
    use crate::c_adapter::cloud_ext_types::{OhCloudExtValueNew, OhCloudExtValueType};
    use crate::service_impl;
    use std::ptr::null;

    macro_rules! ut_vec {
        (
            $typ: ident,
            $push: ident,
            $value_typ: ty,
            $value: ident
        ) => {
            let typ = OhCloudExtRustType::$typ;
            let new_vec = OhCloudExtVectorNew(typ);
            assert!(!new_vec.is_null());
            match OhCloudExtVectorGetValueTyp(new_vec) {
                OhCloudExtRustType::$typ => {}
                _ => panic!("Vector Type wrong"),
            }

            let mut length: u32 = 1;
            assert_eq!(
                OhCloudExtVectorGetLength(new_vec, &mut length as *mut _ as *mut c_uint),
                ERRNO_SUCCESS
            );
            assert_eq!(length, 0);

            assert_eq!(
                // Length of test string "hello"
                OhCloudExtVectorPush(new_vec, $push as *const _ as *mut c_void, 5),
                ERRNO_SUCCESS
            );
            assert_eq!(
                OhCloudExtVectorGetLength(new_vec, &mut length as *mut _ as *mut c_uint),
                ERRNO_SUCCESS
            );
            assert_eq!(length, 1);

            let mut val = null();
            assert_eq!(
                OhCloudExtVectorGet(
                    new_vec,
                    0,
                    &mut val as *mut _ as *mut *const c_void,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            let typ = OhCloudExtRustType::$typ;
            match typ {
                OhCloudExtRustType::VEC_U32 => {
                    let value_struct = OhCloudExtVector::get_inner(
                        val as *mut OhCloudExtVector,
                        SafetyCheckId::Vector,
                    )
                    .unwrap();
                    match value_struct {
                        // ut_vec_vecu32
                        VectorCffi::U32(v) => assert_eq!(*v, vec![3_u32]),
                        _ => panic!("Vector type mismatches with VecU32."),
                    }
                }
                OhCloudExtRustType::STRING => {
                    let slice = &(*slice_from_raw_parts(val as *const u8, length as usize));
                    assert_eq!(slice, b"hello");
                }
                OhCloudExtRustType::VALUE => {
                    let v = OhCloudExtValue::get_inner(
                        val as *mut OhCloudExtValue,
                        SafetyCheckId::Value,
                    )
                    .unwrap();
                    assert_eq!(v, service_impl::types::Value::Empty);
                }
                _ => {
                    let value = &*(val as *const $value_typ);
                    assert_eq!(value, &$value);
                }
            }
            OhCloudExtVectorFree(new_vec);
        };
    }

    /// UT test for Vec<i32> creation and destruction.
    ///
    /// # Title
    /// ut_vec_i32
    ///
    /// # Brief
    /// 1. Create a vec of i32.
    /// 2. Push and get values from it.
    /// 3. Free vec.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_vec_i32() {
        unsafe {
            let src: i32 = 3;
            let borrow = &src;
            ut_vec!(I32, borrow, i32, src);
        }
    }

    /// UT test for Vec<u32> creation and destruction.
    ///
    /// # Title
    /// ut_vec_u32
    ///
    /// # Brief
    /// 1. Create a vec of u32.
    /// 2. Push and get values from it.
    /// 3. Free vec.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_vec_u32() {
        unsafe {
            let src: u32 = 3;
            let borrow = &src;
            ut_vec!(U32, borrow, u32, src);
        }
    }

    /// UT test for Vec<String> creation and destruction.
    ///
    /// # Title
    /// ut_vec_string
    ///
    /// # Brief
    /// 1. Create a vec of string.
    /// 2. Push and get values from it.
    /// 3. Free vec.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_vec_string() {
        unsafe {
            let src = "hello".to_string();
            let borrow = src.as_ptr();
            ut_vec!(STRING, borrow, String, src);
        }
    }

    /// UT test for Vec<Value> creation and destruction.
    ///
    /// # Title
    /// ut_vec_u32
    ///
    /// # Brief
    /// 1. Create a vec of Value.
    /// 2. Push and get values from it.
    /// 3. Free vec.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_vec_value() {
        unsafe {
            let src = OhCloudExtValueNew(OhCloudExtValueType::EMPTY, null_mut(), 0);
            let cmp = types::Value::Empty;
            assert!(!src.is_null());
            ut_vec!(VALUE, src, types::Value, cmp);
        }
    }

    /// UT test for Vec<Vec<u32>> creation and destruction.
    ///
    /// # Title
    /// ut_vec_vecu32
    ///
    /// # Brief
    /// 1. Create a vec of Vec<u32>.
    /// 2. Push and get values from it.
    /// 3. Free vec.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_vec_vecu32() {
        unsafe {
            let vec = OhCloudExtVectorNew(OhCloudExtRustType::U32);
            let mut src: u32 = 3;
            assert_eq!(
                OhCloudExtVectorPush(vec, &mut src as *mut _ as *mut c_void, 1),
                ERRNO_SUCCESS
            );

            let src = vec![3_u32];
            ut_vec!(VEC_U32, vec, Vec<u32>, src);
        }
    }

    /// UT test for vec use and create.
    ///
    /// # Title
    /// ut_vec_null
    ///
    /// # Brief
    /// 1. Pass in null ptr into vec cffi func.
    /// 2. No error and memory leak should happen.
    #[test]
    fn ut_vec_null() {
        unsafe {
            let mut src = 3;
            assert!(OhCloudExtVectorNew(OhCloudExtRustType::NULL).is_null());
            assert_eq!(
                OhCloudExtVectorPush(null_mut(), &mut src as *mut _ as *mut c_void, 0),
                ERRNO_NULLPTR
            );

            let typ = OhCloudExtRustType::I32;
            let new_vec = OhCloudExtVectorNew(typ);
            let mut length = 1;
            assert_eq!(
                OhCloudExtVectorGetLength(null_mut(), &mut length as *mut _ as *mut c_uint),
                ERRNO_NULLPTR
            );
            assert_eq!(OhCloudExtVectorPush(new_vec, null_mut(), 0), ERRNO_NULLPTR);
            OhCloudExtVectorFree(new_vec);
        }
    }

    macro_rules! ut_hashmap {
        (
            $typ: ident,
            $push: ident,
            $value_typ: ty,
            $value: ident
        ) => {
            let typ = OhCloudExtRustType::$typ;
            let new_map = OhCloudExtHashMapNew(typ);
            assert!(!new_map.is_null());
            match OhCloudExtHashMapGetKeyTyp(new_map) {
                OhCloudExtRustType::STRING => {}
                _ => panic!("Hashmap key type wrong"),
            }
            match OhCloudExtHashMapGetValueTyp(new_map) {
                OhCloudExtRustType::$typ => {}
                _ => panic!("Hashmap value wrong"),
            }

            let mut length = 1;
            assert_eq!(
                OhCloudExtHashMapGetLength(new_map, &mut length as *mut _ as *mut c_uint),
                ERRNO_SUCCESS
            );
            assert_eq!(length, 0);

            let key = "key";
            assert_eq!(
                OhCloudExtHashMapInsert(
                    new_map,
                    key.as_ptr() as *mut c_uchar,
                    key.len() as c_uint,
                    $push as *const _ as *mut c_void,
                    0
                ),
                ERRNO_SUCCESS
            );
            assert_eq!(
                OhCloudExtHashMapGetLength(new_map, &mut length as *mut _ as *mut c_uint),
                ERRNO_SUCCESS
            );
            assert_eq!(length, 1);

            let mut key_ptr: *mut OhCloudExtVector = null_mut();
            let mut value_ptr: *mut OhCloudExtVector = null_mut();
            assert_eq!(
                OhCloudExtHashMapIterGetKeyValuePair(
                    new_map,
                    (&mut key_ptr) as *mut _ as *mut *const OhCloudExtVector,
                    (&mut value_ptr) as *mut _ as *mut *const OhCloudExtVector,
                ),
                ERRNO_SUCCESS
            );
            assert!(!key_ptr.is_null());
            assert!(!value_ptr.is_null());

            let mut val = null();
            assert_eq!(
                OhCloudExtVectorGet(
                    key_ptr,
                    0,
                    &mut val as *mut _ as *mut *const c_void,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );

            let key_slice = &*slice_from_raw_parts(val as *const u8, length);
            assert_eq!(key_slice, b"key");
            OhCloudExtVectorFree(key_ptr);

            assert_eq!(
                OhCloudExtVectorGet(
                    value_ptr,
                    0,
                    &mut val as *mut _ as *mut *const c_void,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            match OhCloudExtVectorGetValueTyp(value_ptr) {
                OhCloudExtRustType::VEC_U32 => {
                    let value_struct = OhCloudExtVector::get_inner(
                        val as *mut OhCloudExtVector,
                        SafetyCheckId::Vector,
                    )
                    .unwrap();
                    match value_struct {
                        // ut_vec_vecu32
                        VectorCffi::U32(v) => assert_eq!(*v, vec![3]),
                        _ => panic!("Vector type mismatches with VecU32."),
                    }
                }
                _ => {
                    let value = &*(val as *const $value_typ);
                    assert_eq!(value, $value);
                }
            }
            OhCloudExtVectorFree(value_ptr);

            let mut val = null();
            let mut length = 1;
            assert_eq!(
                OhCloudExtHashMapGet(
                    new_map,
                    key.as_ptr() as *const c_uchar,
                    key.len() as c_uint,
                    &mut val as *mut _ as *mut *const c_void,
                    &mut length as *mut _ as *mut c_uint
                ),
                ERRNO_SUCCESS
            );
            match OhCloudExtHashMapGetValueTyp(new_map) {
                OhCloudExtRustType::VEC_U32 => {
                    let value_struct = OhCloudExtVector::get_inner(
                        val as *mut OhCloudExtVector,
                        SafetyCheckId::Vector,
                    )
                    .unwrap();
                    match value_struct {
                        // ut_hashmap_vecu32
                        VectorCffi::U32(v) => assert_eq!(*v, vec![3_u32]),
                        _ => panic!("Vector type mismatches with VecU32."),
                    }
                }
                _ => {
                    let value = &*(val as *const $value_typ);
                    assert_eq!(value, $value);
                }
            }

            OhCloudExtHashMapFree(new_map);
        };
    }

    /// UT test for hashmap use and create.
    ///
    /// # Title
    /// ut_hashmap_u32
    ///
    /// # Brief
    /// 1. Create a HashMap<String, u32>.
    /// 2. Push and get values from it.
    /// 3. Free vec.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_hashmap_u32() {
        unsafe {
            let src: u32 = 3;
            let borrow = &src;
            ut_hashmap!(U32, borrow, u32, borrow);
        }
    }

    /// UT test for hashmap use and create.
    ///
    /// # Title
    /// ut_hashmap_u32
    ///
    /// # Brief
    /// 1. Create a HashMap<String, Vec<u32>>.
    /// 2. Push and get values from it.
    /// 3. Free vec.
    /// 4. No error and memory leak should happen.
    #[test]
    fn ut_hashmap_vecu32() {
        unsafe {
            let vec = OhCloudExtVectorNew(OhCloudExtRustType::U32);
            let mut src: u32 = 3;
            assert_eq!(
                OhCloudExtVectorPush(vec, &mut src as *mut _ as *mut c_void, 1),
                ERRNO_SUCCESS
            );

            let src = vec![3_u32];
            let borrow = &src;
            ut_hashmap!(VEC_U32, vec, Vec<u32>, borrow);
        }
    }

    /// UT test for hashmap use and create.
    ///
    /// # Title
    /// ut_hashmap_null
    ///
    /// # Brief
    /// 1. Pass in null ptr into hashmap cffi func.
    /// 2. No error and memory leak should happen.
    #[test]
    fn ut_hashmap_null() {
        unsafe {
            assert!(OhCloudExtHashMapNew(OhCloudExtRustType::NULL).is_null());
            let typ = OhCloudExtHashMapGetKeyTyp(null_mut());
            match typ {
                OhCloudExtRustType::NULL => {}
                _ => panic!("Get value type from null ptr is not ValueType::NULL"),
            }
            let typ = OhCloudExtHashMapGetValueTyp(null_mut());
            match typ {
                OhCloudExtRustType::NULL => {}
                _ => panic!("Get value type from null ptr is not ValueType::NULL"),
            }

            let typ = OhCloudExtRustType::U32;
            let new_map = OhCloudExtHashMapNew(typ);
            let mut length = 0;
            assert!(!new_map.is_null());
            let mut src = 3_u8;
            assert_eq!(
                OhCloudExtHashMapInsert(
                    null_mut(),
                    "key".as_ptr() as *mut c_uchar,
                    3,
                    &mut src as *mut _ as *mut c_void,
                    0
                ),
                ERRNO_NULLPTR
            );
            assert_eq!(
                OhCloudExtHashMapGetLength(null_mut(), &mut length as *mut _ as *mut c_uint),
                ERRNO_NULLPTR
            );
            OhCloudExtHashMapFree(new_map);
        }
    }
}
