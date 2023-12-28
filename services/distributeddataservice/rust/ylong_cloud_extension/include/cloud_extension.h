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

/**
 * @defgroup    cloud_extension C headers
 * @ingroup     cloud_extension
 */

#ifndef CLOUD_EXTENSION_H
#define CLOUD_EXTENSION_H

#ifndef CLOUD_EXTENSION_BASIC_RUST_TYPES_H
#include "basic_rust_types.h"
#endif

#ifndef CLOUD_EXTENSION_TYPES_H
#include "cloud_ext_types.h"
#endif

#ifndef CLOUD_EXTENSION_ERROR_H
#include "error.h"
#endif

#include <stddef.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// CloudAssetLoader, CloudDatabase, CloudSync
/**
 * @brief       Type declaration of Rust cloud extension struct CloudAssetLoader.
 * @attention   When an instance is created by CloudAssetLoaderNew，memory is managed by Rust and
 *              it will return a pointer. When destroying this instance, users should call
 *              CloudAssetLoaderFree to release memory and prevent leak.
 *
 *              Besides, CloudAssetLoader is only valid when the database passed in its initialization
 *              function is valid.
 */
typedef struct {
    const size_t id;
} OhCloudExtCloudAssetLoader;

/**
 * @brief       Information needed when upload or download assets through CloudAssetLoader.
 */
typedef struct {
    const unsigned char *tableName;
    const unsigned int tableNameLen;
    const unsigned char *gid;
    const unsigned int gidLen;
    const unsigned char *prefix;
    const unsigned int prefixLen;
} OhCloudExtUpDownloadInfo;

/**
 * @brief       Create an CloudAssetLoader instance.
 * @param       userId     [IN]
 *              bundleName [IN]
 *              nameLen    [IN]
 *              db         [IN]
 */
OhCloudExtCloudAssetLoader *OhCloudExtCloudAssetLoaderNew(
    int userId,
    const unsigned char *bundleName,
    unsigned int nameLen,
    OhCloudExtDatabase* db
);

/**
 * @brief       Upload assets.
 * @param       loader          [IN]
 *              tableName       [IN]
 *              tableNameLen    [IN]
 *              gid             [IN]
 *              gidLen          [IN]
 *              prefix          [IN]
 *              prefixLen       [IN]
 *              assets          [IN/OUT] HashMap<String, Value>
 * @attention   The Vector returned should be freed by `OhCloudExtVectorFree` if no longer in use.
 */
int OhCloudExtCloudAssetLoaderUpload(
    OhCloudExtCloudAssetLoader *loader,
    const OhCloudExtUpDownloadInfo *info,
    OhCloudExtVector *assets
);

/**
 * @brief       Download assets.
 * @param       loader          [IN]
 *              tableName       [IN]
 *              tableNameLen    [IN]
 *              gid             [IN]
 *              gidLen          [IN]
 *              prefix          [IN]
 *              prefixLen       [IN]
 *              assets          [IN/OUT] HashMap<String, Value>
 * @attention   The Vector returned should be freed by `OhCloudExtVectorFree` if no longer in use.
 */
int OhCloudExtCloudAssetLoaderDownload(
    OhCloudExtCloudAssetLoader *loader,
    const OhCloudExtUpDownloadInfo *info,
    OhCloudExtVector *assets
);

/**
 * @brief       Remove one asset from the local path.
 * @param       asset           [IN]
 */
int OhCloudExtCloudAssetLoaderRemoveLocalAssets(const OhCloudExtCloudAsset *asset);

/**
 * @brief       Destroy an CloudAssetLoader instance.
 * @param       ptr     [IN]    The pointer of CloudAssetLoader that should be destroyed.
 */
void OhCloudExtCloudAssetLoaderFree(OhCloudExtCloudAssetLoader *ptr);

/**
 * @brief       Type declaration of Rust cloud extension struct CloudDatabase.
 * @attention   When an instance is created by CloudDbNew，memory is managed by Rust and it will return a pointer.
 *              When destroying this instance, users should call CloudDatabaseFree to release memory and prevent leak.
 */
typedef struct {
    const size_t id;
} OhCloudExtCloudDatabase;

/**
 * @brief       Create an CloudDatabase instance.
 * @retval      != NULL, a valid pointer of CloudDatabase.
 * @attention   Database passed in will be managed by CloudDb instead. No free function is needed to call on it.
 */
OhCloudExtCloudDatabase *OhCloudExtCloudDbNew(
    int userId,
    const unsigned char *bundleName,
    unsigned int nameLen,
    OhCloudExtDatabase *database
);

/**
 * @brief       Sql that will passed to CloudDatabase and executed.
 */
typedef struct {
    const unsigned char *table;
    const unsigned int tableLen;
    const unsigned char *sql;
    const unsigned int sqlLen;
} OhCloudExtSql;

/**
 * @brief       Execute a sql on a CloudDatabase instance.
 * @param       cdb         [IN]
 *              table       [IN]
 *              tableLen    [IN]
 *              sql         [IN]
 *              sqlLen      [IN]
 *              extend      [IN/OUT] HashMap<String, Value>
 * @retval      SUCCESS
 *              ERRNO_Unsupported
 */
int OhCloudExtCloudDbExecuteSql(
    OhCloudExtCloudDatabase *cdb,
    const OhCloudExtSql *sql,
    OhCloudExtHashMap *extend
);

/**
 * @brief       Insert batches of value buckets into a CloudDatabase instance.
 * @param       cdb         [IN]
 *              table       [IN]
 *              tableLen    [IN]
 *              value       [IN]     Vec<HashMap<String, Value>>
 *              extend      [IN/OUT] Vec<HashMap<String, Value>>
 * @retval      SUCCESS
 */
int OhCloudExtCloudDbBatchInsert(
    OhCloudExtCloudDatabase *cdb,
    const unsigned char *table,
    unsigned int tableLen,
    const OhCloudExtVector *value,
    OhCloudExtVector *extend
);

/**
 * @brief       Update batches of value buckets in a CloudDatabase instance.
 * @param       cdb         [IN]
 *              table       [IN]
 *              tableLen    [IN]
 *              value       [IN]     Vec<HashMap<String, Value>>
 *              extend      [IN/OUT] Vec<HashMap<String, Value>>
 * @retval      SUCCESS
 */
int OhCloudExtCloudDbBatchUpdate(
    OhCloudExtCloudDatabase *cdb,
    const unsigned char *table,
    unsigned int tableLen,
    const OhCloudExtVector *value,
    OhCloudExtVector *extend
);

/**
 * @brief       Delete batches of value buckets from a CloudDatabase instance.
 * @param       cdb         [IN]
 *              table       [IN]
 *              tableLen    [IN]
 *              value       [IN]     Vec<HashMap<String, Value>>
 *              extend      [IN/OUT] Vec<HashMap<String, Value>>
 * @retval      SUCCESS
 */
int OhCloudExtCloudDbBatchDelete(
    OhCloudExtCloudDatabase *cdb,
    const unsigned char *table,
    unsigned int tableLen,
    const OhCloudExtVector *extend
);

/**
 * @brief       Query info that will passed to CloudDatabase.
 */
typedef struct {
    const unsigned char *table;
    const unsigned int tableLen;
    const unsigned char *cursor;
    const unsigned int cursorLen;
} OhCloudExtQueryInfo;

/**
 * @brief       Search batches of value buckets from a CloudDatabase instance.
 * @param       cdb         [IN]
 *              table       [IN]
 *              tableLen    [IN]
 *              cursor      [IN]
 *              cursorLen   [IN]
 *              out         [OUT]       The address of a pointer to a Cursor. The pointer value will be updated when
 *                                      query function successfully returns.
 * @attention   The CloudDbData returned should be freed by `OhCloudExtCloudDbDataFree` if no longer in use.
 */
int OhCloudExtCloudDbBatchQuery(
    OhCloudExtCloudDatabase *cdb,
    const OhCloudExtQueryInfo *info,
    OhCloudExtCloudDbData **out
);

/**
 * @brief       Lock a CloudDatabase instance.
 * @param       cdb         [IN]
 *              expire      [OUT]
 */
int OhCloudExtCloudDbLock(OhCloudExtCloudDatabase *cdb, int *expire);

/**
 * @brief       Unlock a CloudDatabase instance.
 * @param       cdb         [IN]
 */
int OhCloudExtCloudDbUnlock(OhCloudExtCloudDatabase *cdb);

/**
 * @brief       Heartbeat function of a CloudDatabase. Extend the current locking session.
 * @param       cdb         [IN]
 */
int OhCloudExtCloudDbHeartbeat(OhCloudExtCloudDatabase *cdb);

/**
 * @brief       Destroy an CloudDatabase instance.
 * @param       ptr     [IN]    The pointer of CloudDatabase that should be destroyed.
 */
void OhCloudExtCloudDbFree(OhCloudExtCloudDatabase *ptr);

/**
 * @brief       Type declaration of Rust cloud extension struct CloudSync.
 * @attention   When an instance is created by CloudSyncNew，memory is managed by Rust and it will return a pointer.
 *              When destroying this instance, users should call CloudSyncFree to release memory and prevent leak.
 */
typedef struct {
    const size_t id;
} OhCloudExtCloudSync;

/**
 * @brief       Create an CloudSync instance.
 * @param       userId     [IN]
 * @retval      != NULL, a valid pointer of CloudSync.
 */
OhCloudExtCloudSync *OhCloudExtCloudSyncNew(int userId);

/**
 * @brief       Get service info from a CloudSync pointer.
 * @param       server     [IN]
 *              info       [OUT]
 * @attention   The CloudInfo returned should be freed by `CloudInfoFree` if no longer in use.
 */
int OhCloudExtCloudSyncGetServiceInfo(OhCloudExtCloudSync *server, OhCloudExtCloudInfo **info);

/**
 * @brief       Get app schema from a CloudSync pointer, with a bundle name.
 * @param       server          [IN]
 *              bundleName      [IN]
 *              bundleNameLen   [IN]
 *              schema          [OUT]
 * @attention   The SchemaMeta returned should be freed by `SchemaMetaFree` if no longer in use.
 */
int OhCloudExtCloudSyncGetAppSchema(
    OhCloudExtCloudSync *server,
    const unsigned char *bundleName,
    unsigned int bundleNameLen,
    OhCloudExtSchemaMeta **schema
);

/**
 * @brief       Pass a batch of subscription orders to a CloudSync pointer, with target database information
 *              and expected expire time.
 * @param       server      [IN]
 *              dbs         [IN]  HashMap<String, Vec<Databases>>, bundle name as key
 *              relations   [OUT] HashMap<String, RelationSet>, bundle name as key
 *              err         [OUT] Vec<I32>
 * @attention   The Vector returned should be freed by `OhCloudExtVectorFree`. Similar for the HashMap.
 */
int OhCloudExtCloudSyncSubscribe(
    OhCloudExtCloudSync *server,
    const OhCloudExtHashMap *dbs,
    unsigned long long expire,
    OhCloudExtHashMap **relations,
    OhCloudExtVector **err
);

/**
 * @brief       Pass a batch of unsubscription orders to a CloudSync pointer, with target relations.
 * @param       server          [IN]
 *              relations       [IN] HashMap<String, Vec<String>>, bundle name as key, ids as value
 *              err             [OUT] Vec<I32>
 * @attention   The Vector returned should be freed by `OhCloudExtVectorFree`.
 */
int OhCloudExtCloudSyncUnsubscribe(
    OhCloudExtCloudSync *server,
    const OhCloudExtHashMap *relations,
    OhCloudExtVector **err
);

/**
 * @brief       Destroy an CloudSync instance.
 * @param       ptr     [IN]    The pointer of CloudSync that should be destroyed.
 */
void OhCloudExtCloudSyncFree(OhCloudExtCloudSync *ptr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif // CLOUD_EXTENSION_H