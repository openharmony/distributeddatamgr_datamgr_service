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

#ifndef CLOUD_EXTENSION_BASIC_RUST_TYPES_H
#define CLOUD_EXTENSION_BASIC_RUST_TYPES_H

#ifndef CLOUD_EXTENSION_ERROR_H
#include "error.h"
#endif

#include <stddef.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

enum OhCloudExtRustType {
    VALUETYPE_NULL,
    VALUETYPE_U32,
    VALUETYPE_I32,
    VALUETYPE_STRING,
    VALUETYPE_VALUE,
    VALUETYPE_VALUE_BUCKET,
    VALUETYPE_DATABASE,
    VALUETYPE_TABLE,
    VALUETYPE_FIELD,
    VALUETYPE_RELATION,
    VALUETYPE_RELATION_SET,
    VALUETYPE_CLOUD_ASSET,
    VALUETYPE_APP_INFO,
    VALUETYPE_VEC_U32,
    VALUETYPE_VEC_STRING,
    VALUETYPE_VEC_DATABASE,
    VALUETYPE_HASHMAP_VALUE,
};

/**
* @brief       Type declaration of Rust struct OhCloudExtVector.
* @attention   The memory is managed by Rust. Therefore, to memory leaks, users should call `OhCloudExtVectorFree`
*              to release the memory it occupies.
*/
typedef struct {
    const size_t id;
} OhCloudExtVector;

/**
 * @brief       Create a OhCloudExtVector enum according to OhCloudExtRustType.
 * @attention   The memory is managed by Rust. Therefore, to memory leaks, users should call `OhCloudExtVectorFree`
 *              to release the memory it occupies.
 */
OhCloudExtVector *OhCloudExtVectorNew(OhCloudExtRustType typ);

/**
 * @breif       Get OhCloudExtRustType of the OhCloudExtVector pointer.
 */
OhCloudExtRustType OhCloudExtVectorGetValueTyp(OhCloudExtVector *src);

/**
 * @breif       Push value into the OhCloudExtVector pointer.
 * @attention   For Values pushed in, if they're created by other functions and allocated in Rust side,
 *              pushing them will transfer the management to the OhCloudExtVector. Therefore, no more free is needed.
 */
int OhCloudExtVectorPush(
    OhCloudExtVector *src,
    void *value,
    unsigned int valueLen
);

/**
 * @breif       Get value from the OhCloudExtVector pointer.
 * @attention   If value type is not raw C type, value will be copied so that the C side can get information stored
 *              in Rust side. In this case, the pointer returned back should be freed by corresponding free function.
 */
int OhCloudExtVectorGet(
    const OhCloudExtVector *src,
    unsigned int idx,
    void **value,
    unsigned int *valueLen
);

/**
 * @brief       Get length of a OhCloudExtVector pointer.
 */
int OhCloudExtVectorGetLength(const OhCloudExtVector *src, unsigned int *len);

/**
 * @brief       Free a OhCloudExtVector pointer.
 */
void OhCloudExtVectorFree(OhCloudExtVector *src);

/**
* @brief       Type declaration of Rust struct OhCloudExtHashMap.
* @attention   The memory is managed by Rust. Therefore, to memory leaks, users should call `OhCloudExtHashMapFree`
*              to release the memory it occupies.
*/
typedef struct {
    const size_t id;
} OhCloudExtHashMap;

/**
 * @brief       Initialize a OhCloudExtHashMap enum by its OhCloudExtRustType. The key type is fixed to be String.
 * @attention   The memory is managed by Rust. Therefore, to memory leaks, users should call `OhCloudExtHashMapFree`
 *              to release the memory it occupies.
 */
OhCloudExtHashMap *OhCloudExtHashMapNew(OhCloudExtRustType valueTyp);

/**
 * @brief       Get key type of a OhCloudExtHashMap pointer.
 */
OhCloudExtRustType OhCloudExtHashMapGetKeyTyp(const OhCloudExtHashMap *src);

/**
 * @brief       Get value type of a OhCloudExtHashMap pointer.
 */
OhCloudExtRustType OhCloudExtHashMapGetValueTyp(const OhCloudExtHashMap *src);

/**
 * @brief       Get length of a OhCloudExtHashMap pointer.
 */
int OhCloudExtHashMapGetLength(const OhCloudExtHashMap *src, unsigned int *len);

/**
 * @brief       Insert key, value pairs into the OhCloudExtHashMap pointer.
 * @attention   For Values pushed in, if they're created by other functions and allocated in Rust side,
 *              pushing them will transfer the management to the OhCloudExtHashMap. Therefore, no more free is needed.
 */
int OhCloudExtHashMapInsert(
    OhCloudExtHashMap *src,
    void *key,
    unsigned int keyLen,
    void *value,
    unsigned int valueLen
);

/**
 * @brief       Get key, value pair from the OhCloudExtHashMap pointer.
 * @param       src         [IN]
 *              key         [OUT]  Vec<KeyType>, here fixed to be Vec<String> now
 *              value       [OUT]  Vec<OhCloudExtRustType>
 * @retval      SUCCESS
 *              ERRNO_NULLPTR
 * @attention   The vectors output should be freed by VectorFree.
 */
int OhCloudExtHashMapIterGetKeyValuePair(
    const OhCloudExtHashMap *src,
    OhCloudExtVector **key,
    OhCloudExtVector **value
);

/**
 * @brief       According to key, get value from the OhCloudExtHashMap pointer.
 * @attention   If value type is not raw C type, value will be copied so that the C side can get information stored
 *              in Rust side. In this case, the pointer returned back should be freed by corresponding free function.
 */
int OhCloudExtHashMapGet(
    const OhCloudExtHashMap *src,
    const void *key,
    unsigned int keyLen,
    void **value,
    unsigned int *valueLen
);

/**
 * @brief       Free a OhCloudExtHashMap pointer.
 */
void OhCloudExtHashMapFree(OhCloudExtHashMap *src);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // CLOUD_EXTENSION_BASIC_RUST_TYPES_H
