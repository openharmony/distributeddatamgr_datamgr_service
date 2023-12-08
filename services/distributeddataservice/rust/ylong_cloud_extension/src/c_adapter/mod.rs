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

// Mask for cffi purpose.
#![allow(non_camel_case_types)]

pub(crate) mod basic_rust_types;
pub(crate) mod cloud_ext_types;
pub(crate) mod cloud_extension;

use basic_rust_types::*;
use cloud_ext_types::*;

use std::ffi::{c_int, c_uchar, c_uint, c_void};
use std::ptr::slice_from_raw_parts;

/// Errno of success.
pub const ERRNO_SUCCESS: c_int = 0;
/// Errno of null pointer.
pub const ERRNO_NULLPTR: c_int = 1;
/// Errno of wrong type.
pub const ERRNO_WRONG_TYPE: c_int = 2;
/// Errno of invalid input type.
pub const ERRNO_INVALID_INPUT_TYPE: c_int = 3;
/// Errno of asset download failure.
pub const ERRNO_ASSET_DOWNLOAD_FAILURE: c_int = 4;
/// Errno of asset upload failure.
pub const ERRNO_ASSET_UPLOAD_FAILURE: c_int = 5;
/// Errno of unsupported feature.
pub const ERRNO_UNSUPPORTED: c_int = 6;
/// Errno of network error.
pub const ERRNO_NETWORK_ERROR: c_int = 7;
/// Errno of lock conflict.
pub const ERRNO_LOCKED_BY_OTHERS: c_int = 8;
/// Errno of locking action on unlocked cloud database.
pub const ERRNO_UNLOCKED: c_int = 9;
/// Errno of record limit exceeding.
pub const ERRNO_RECORD_LIMIT_EXCEEDED: c_int = 10;
/// Errno of no space for assets.
pub const ERRNO_NO_SPACE_FOR_ASSET: c_int = 11;
/// Errno of ipc read / write parcel error.
pub const ERRNO_IPC_RW_ERROR: c_int = 12;
/// Errno of IPC connection error.
pub const ERRNO_IPC_CONN_ERROR: c_int = 13;
/// Errno of batch of ipc errors.
pub const ERRNO_IPC_ERRORS: c_int = 14;
/// Errno of other io error.
pub const ERRNO_OTHER_IO_ERROR: c_int = 15;
/// Errno of array out of range.
pub const ERRNO_OUT_OF_RANGE: c_int = 16;
/// Errno of no target table in source database.
pub const ERRNO_NO_SUCH_TABLE_IN_DB: c_int = 17;
/// Errno of invalid status of the cloud
pub const ERRNO_CLOUD_INVALID_STATUS: c_int = 18;
/// Errno of unknown error.
pub const ERRNO_UNKNOWN: c_int = 19;
/// Errno of cloud disabling
pub const ERRNO_CLOUD_DISABLE: c_int = 20;
/// Errno of invalid key
pub const ERRNO_INVALID_KEY: c_int = 21;

#[derive(Copy, Clone)]
pub(crate) enum SafetyCheckId {
    // Arbitrarily defined id, used for type safety check
    Vector = 32_isize,
    HashMap = 33,
    Value = 34,
    CloudAsset = 35,
    Database = 36,
    CloudInfo = 37,
    AppInfo = 38,
    Table = 39,
    Field = 40,
    SchemaMeta = 41,
    RelationSet = 42,
    CloudDbData = 43,
    ValueBucket = 44,
    CloudAssetLoader = 45,
    CloudDatabase = 46,
    CloudSync = 47,
}

impl SafetyCheckId {
    pub(crate) fn as_usize(&self) -> usize {
        *self as usize
    }
}

pub(crate) unsafe fn type_safety_check(ptr: *const c_void, tar_type: SafetyCheckId) -> bool {
    if ptr.is_null() {
        return false;
    }
    // Read the id, if possible
    let src = &*(ptr as *const usize);
    *src == tar_type.as_usize()
}

/// Wrapper of Rust struct. Add id for safety requirements, pointers passed in will be checked
/// according to this id.
#[repr(C)]
pub struct SafeCffiWrapper<T> {
    id: usize,
    inner: T,
}

impl<T> SafeCffiWrapper<T> {
    pub(crate) fn new(inner: T, tar_type: SafetyCheckId) -> SafeCffiWrapper<T> {
        SafeCffiWrapper {
            id: tar_type.as_usize(),
            inner,
        }
    }

    pub(crate) fn into_ptr(self) -> *mut SafeCffiWrapper<T> {
        Box::into_raw(Box::new(self))
    }

    pub(crate) unsafe fn from_ptr(
        src: *mut SafeCffiWrapper<T>,
        typ: SafetyCheckId,
    ) -> Option<SafeCffiWrapper<T>> {
        src.as_mut().map(|obj| {
            type_safety_check(obj as *const _ as *const c_void, typ)
                .then_some(*Box::from_raw(obj as *mut SafeCffiWrapper<T>))
        })?
    }

    pub(crate) unsafe fn get_inner_ref(
        src: *const SafeCffiWrapper<T>,
        typ: SafetyCheckId,
    ) -> Option<&'static T> {
        if let Some(obj) = src.as_ref() {
            if type_safety_check(obj as *const _ as *const c_void, typ) {
                let wrapper = &*src;
                return Some(&wrapper.inner);
            }
        }
        None
    }

    pub(crate) unsafe fn get_inner_mut(
        src: *mut SafeCffiWrapper<T>,
        typ: SafetyCheckId,
    ) -> Option<&'static mut T> {
        if let Some(obj) = src.as_mut() {
            if type_safety_check(obj as *const _ as *const c_void, typ) {
                let wrapper = &mut *src;
                return Some(&mut wrapper.inner);
            }
        }
        None
    }

    pub(crate) unsafe fn get_inner(src: *mut SafeCffiWrapper<T>, typ: SafetyCheckId) -> Option<T> {
        SafeCffiWrapper::<T>::from_ptr(src, typ).map(|x| x.inner)
    }
}

/// Transform a pointer of c char array and its length to a String.
///
/// # Safety
/// Users should guarantee that String doesn't contain non-UTF8 literals, and the length passed
/// in is valid to avoid memory issues and undefined behaviors.
pub(crate) unsafe fn char_ptr_to_string(ptr: *const c_uchar, len: c_uint) -> String {
    if ptr.is_null() {
        return String::default();
    }
    let slice = &(*slice_from_raw_parts(ptr, len as usize));
    String::from_utf8_unchecked(slice.to_vec())
}

#[cfg(test)]
mod test {
    use crate::c_adapter::*;
    use std::collections::HashMap;

    /// UT test for type wrapper.
    ///
    /// # Title
    /// ut_wrapper
    ///
    /// # Brief
    /// 1. Pass in type with wrong id.
    /// 2. Process should not panic, but return Option successfully.
    #[test]
    fn ut_wrapper() {
        unsafe {
            let mut src = 3;
            let mut wrong_typ = 1_usize;
            assert_eq!(
                OhCloudExtVectorPush(
                    &mut wrong_typ as *mut _ as *mut OhCloudExtVector,
                    &mut src as *mut _ as *mut c_void,
                    0
                ),
                ERRNO_WRONG_TYPE
            );

            let map = HashMapCffi::U32(HashMap::new());
            let mut wrong_map = OhCloudExtHashMap::new(map, SafetyCheckId::HashMap);
            assert_eq!(
                OhCloudExtVectorPush(
                    &mut wrong_map as *mut _ as *mut OhCloudExtVector,
                    &mut src as *mut _ as *mut c_void,
                    0
                ),
                ERRNO_WRONG_TYPE
            );

            let vec = OhCloudExtVectorNew(OhCloudExtRustType::I32);
            assert!(!vec.is_null());
            assert_eq!(
                OhCloudExtVectorPush(vec, &mut src as *mut _ as *mut c_void, 0),
                ERRNO_SUCCESS
            );
            OhCloudExtVectorFree(vec);
        }
    }
}
