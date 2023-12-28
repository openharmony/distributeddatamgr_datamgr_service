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

#ifndef CLOUD_EXTENSION_TYPES_H
#define CLOUD_EXTENSION_TYPES_H

#ifndef CLOUD_EXTENSION_BASIC_RUST_TYPES_H
#include "basic_rust_types.h"
#endif

#include <stddef.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * @brief       Type declaration of Rust cloud extension struct Value.
 * @attention   The memory is managed by Rust. Therefore, to prevent memory leaks, users should call
 *              `OhCloudExtValueFree` to release the memory occupied
 */
typedef struct {
    const size_t id;
} OhCloudExtValue;

/**
 * @brief       Enumeration represents inner type of contents in Value enum.
 */
enum OhCloudExtValueType {
    VALUEINNERTYPE_EMPTY,
    VALUEINNERTYPE_INT,
    VALUEINNERTYPE_FLOAT,
    VALUEINNERTYPE_STRING,
    VALUEINNERTYPE_BOOL,
    VALUEINNERTYPE_BYTES,
    VALUEINNERTYPE_ASSET,
    VALUEINNERTYPE_ASSETS
};

/**
 * @brief       Create an Value instance according to ValueInnerType.
 * @retval      != NULL, a valid pointer of OhCloudExtValue.
 */
OhCloudExtValue *OhCloudExtValueNew(OhCloudExtValueType typ, void *content, unsigned int contentLen);

/**
 * @brief       Get content from a Value pointer.
 * @param       src         [IN]
 *              typ         [OUT] User should use this value to cast the output pointer.
 *              content     [OUT] The pointer value will be updated to point to the content of the Value.
 *              contentLen  [OUT]
 * @attention   If the type is Asset or Assets, the OhCloudExtVector pointer returned should be freed by
 *              OhCloudExtVectorFree.
 */
int OhCloudExtValueGetContent(
    OhCloudExtValue *src,
    OhCloudExtValueType *typ,
    void **content,
    unsigned int *contentLen
);

/**
 * @brief Free a Value pointer.
 */
void OhCloudExtValueFree(OhCloudExtValue *src);

/**
 * @brief       AssetStutus enumeration.
 */
enum OhCloudExtAssetStatus {
    ASSETSTATUS_UNKNOWN,
    ASSETSTATUS_NORMAL,
    ASSETSTATUS_INSERT,
    ASSETSTATUS_UPDATE,
    ASSETSTATUS_DELETE,
    ASSETSTATUS_ABNORMAL,
    ASSETSTATUS_DOWNLOADING,
    ASSETSTATUS_BUTT,
};

/**
* @brief       CloudAsset builder that will used to build an CloudAsset in Rust side
*/
typedef struct {
    const unsigned long long version;
    const OhCloudExtAssetStatus status;
    const unsigned long long expiresTime;
    const unsigned char *id;
    const unsigned int idLen;
    const unsigned char *name;
    const unsigned int nameLen;
    const unsigned char *uri;
    const unsigned int uriLen;
    const unsigned char *localPath;
    const unsigned int localPathLen;
    const unsigned char *createTime;
    const unsigned int createTimeLen;
    const unsigned char *modifyTime;
    const unsigned int modifyTimeLen;
    const unsigned char *size;
    const unsigned int sizeLen;
    const unsigned char *hash;
    const unsigned int hashLen;
} OhCloudExtCloudAssetBuilder;

/**
 * @brief       Type declaration of Rust cloud extension struct CloudAsset.
 * @attention   The memory is managed by Rust. Therefore, to prevent memory leaks, users should call
 *              `OhCloudExtCloudAssetFree` to release the memory occupied
 */
typedef struct {
    const size_t id;
} OhCloudExtCloudAsset;

/**
 * @brief       Initialize an CloudAsset by the AssetBuilder.
 * @attention   The memory is managed by Rust. Therefore, to prevent memory leaks, users should call
 *              `OhCloudExtCloudAssetFree` to release the memory occupied
 */
OhCloudExtCloudAsset *OhCloudExtCloudAssetNew(const OhCloudExtCloudAssetBuilder *builder);

/**
 * @brief       Get id from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetId(const OhCloudExtCloudAsset *asset, unsigned char **id, unsigned int *len);

/**
 * @brief       Get name from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetName(const OhCloudExtCloudAsset *asset, unsigned char **name, unsigned int *len);

/**
 * @brief       Get uri from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetUri(const OhCloudExtCloudAsset *asset, unsigned char **uri, unsigned int *len);

/**
 * @brief       Get local path from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetLocalPath(const OhCloudExtCloudAsset *asset, unsigned char **localPath, unsigned int *len);

/**
 * @brief       Get create time from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetCreateTime(const OhCloudExtCloudAsset *asset, unsigned char **createTime, unsigned int *len);

/**
 * @brief       Get modified time from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetModifiedTime(
    const OhCloudExtCloudAsset *asset, unsigned char **modifiedTime, unsigned int *len);

/**
 * @brief       Get size from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetSize(const OhCloudExtCloudAsset *asset, unsigned char **size, unsigned int *len);

/**
 * @brief       Get hash from an CloudAsset pointer.
 */
int OhCloudExtCloudAssetGetHash(const OhCloudExtCloudAsset *asset, unsigned char **hash, unsigned int *len);

/**
 * @brief Free an CloudAsset pointer.
 */
void OhCloudExtCloudAssetFree(OhCloudExtCloudAsset *src);

/**
 * @brief       Type declaration of Rust cloud extension struct Database. Can be obtained from Schema.
 * @attention   The memory is managed by Rust. Therefore, to prevent memory leaks, users should call `DatabaseFree` to
 *              release the memory occupied
 */
typedef struct {
    const size_t id;
} OhCloudExtDatabase;

/**
 * @brief       Create a Database instance.
 * @param       tables  [IN] HashMap<String, Table>, table_name as key
 * @retval      != NULL, a valid pointer of Database.
 * @attention   When passed in, database will take control of the memory management of the vector tables. No more free
 *              is needed. For the database pointer, the memory is managed by Rust. Therefore, to prevent memory leaks,
 *              users should call `OhCloudExtDatabaseFree` to release the memory occupied
 */
OhCloudExtDatabase *OhCloudExtDatabaseNew(
    const unsigned char *name,
    unsigned int nameLen,
    const unsigned char *alias,
    unsigned int aliasLen,
    OhCloudExtHashMap *tables
);

/**
 * @brief       Get name from a Database instance.
 * @param       db          [IN]
 *              name        [OUT]
 *              len         [OUT]
 * @retval      SUCCESS
 *              ERRNO_NULLPTR
 * @attention   Name returned shouldn't be freed, or issues of double free is possible.
 */
int OhCloudExtDatabaseGetName(const OhCloudExtDatabase *db, unsigned char **name, unsigned int *len);

/**
 * @brief       Get alias from a Database instance.
 * @param       db           [IN]
 *              alias        [OUT]
 *              len          [OUT]
 * @retval      SUCCESS
 *              ERRNO_NULLPTR
 * @attention   Alias returned shouldn't be freed, or issues of double free is possible.
 */
int OhCloudExtDatabaseGetAlias(const OhCloudExtDatabase *db, unsigned char **alias, unsigned int *len);

/**
 * @brief       Get tables from a Database instance.
 * @param       db          [IN]
 *              tables      [OUT] HashMap<String, Table>, table name as key
 * @retval      SUCCESS
 *              ERRNO_NULLPTR
 * @attention   The HashMap returned should be freed by OhCloudExtHashMapFree.
 */
int OhCloudExtDatabaseGetTable(const OhCloudExtDatabase *db, OhCloudExtHashMap **tables);

/**
 * @brief       Free a Database pointer.
 */
void OhCloudExtDatabaseFree(OhCloudExtDatabase *db);

/**
 * @brief       Type declaration of Rust cloud extension struct Table. Can be obtained from Database.
 * @attention   The memory is managed by Rust. Therefore, to prevent memory leaks, users should call
 *              `OhCloudExtTableFree` to release the memory occupied
 */
typedef struct {
    const size_t id;
} OhCloudExtTable;

/**
 * @brief       Create a Table instance.
 *              fields      [IN] Vec<Field>
 * @attention   When passed in, table will take control of the memory management of the vector field. No more free is
 *              needed.
 */
OhCloudExtTable *OhCloudExtTableNew(
    const unsigned char *name,
    unsigned int nameLen,
    const unsigned char *alias,
    unsigned int aliasLen,
    OhCloudExtVector *fields
);

/**
 * @brief       Get name from a Table instance.
 * @param       tb          [IN]
 *              name        [OUT]
 *              len         [OUT]
 * @attention   Name returned shouldn't be freed, or issues of double free is possible.
 */
int OhCloudExtTableGetName(const OhCloudExtTable *tb, unsigned char **name, unsigned int *len);

/**
 * @brief       Get alias from a Table instance.
 * @param       tb          [IN]
 *              alias       [OUT]
 *              len         [OUT]
 * @attention   Alias returned shouldn't be freed, or issues of double free is possible.
 */
int OhCloudExtTableGetAlias(const OhCloudExtTable *tb, unsigned char **alias, unsigned int *len);

/**
 * @brief       Get fields from a Database instance.
 * @param       tb          [IN]
 *              fields      [OUT]  Vec<Field>
 *              len         [OUT]
 * @attention   The Vector  returned should be freed by OhCloudExtVectorFree.
 */
int OhCloudExtTableGetFields(const OhCloudExtTable *tb, OhCloudExtVector **fields);

/**
 * @brief       Free a Table pointer.
 */
void OhCloudExtTableFree(OhCloudExtTable *db);

/**
 * @brief       Type declaration of Rust cloud extension struct Field. Can be obtained from Table.
 * @attention   The memory is managed by Rust. Therefore, to prevent memory leaks, users should call `FieldFree` to
 *              release the memory occupied
 */
typedef struct {
    const size_t id;
} OhCloudExtField;

/**
 * @brief       FieldBuilder providing necessary information when create a Field.
 */
typedef struct {
    const unsigned char *colName;
    const unsigned int colNameLen;
    const unsigned char *alias;
    const unsigned int aliasLen;
    const unsigned int typ;
    const bool primary;
    const bool nullable;
} OhCloudExtFieldBuilder;

/**
 * @brief       Initialize a Field instance.
 * @attention   The memory is managed by Rust. Therefore, to prevent memory leaks, users should call
 *              `OhCloudExtFieldFree` to release the memory occupied
 */
OhCloudExtField *OhCloudExtFieldNew(const OhCloudExtFieldBuilder *builder);

/**
 * @brief       Get name from a Field instance.
 * @param       fd          [IN]
 *              name        [OUT]
 *              len         [OUT]
 * @attention   Name returned shouldn't be freed, or issues of double free is possible.
 */
int OhCloudExtFieldGetColName(const OhCloudExtField *fd, unsigned char **name, unsigned int *len);

/**
 * @brief       Get alias from a Field instance.
 * @param       fd          [IN]
 *              alias       [OUT]
 *              len         [OUT]
 * @attention   Alias returned shouldn't be freed, or issues of double free is possible.
 */
int OhCloudExtFieldGetAlias(const OhCloudExtField *fd, unsigned char **alias, unsigned int *len);

/**
 * @brief       Get type of a Field instance.
 * @param       fd          [IN]
 *              typ         [OUT]
 */
int OhCloudExtFieldGetTyp(const OhCloudExtField *fd, unsigned int *typ);

/**
 * @brief       Get primacy of a Field instance.
 * @param       fd          [IN]
 *              primary     [OUT]
 */
int OhCloudExtFieldGetPrimary(const OhCloudExtField *fd, bool *primary);

/**
 * @brief       Get nullability of a Field instance.
 * @param       fd          [IN]
 *              nullable    [OUT]
 */
int OhCloudExtFieldGetNullable(const OhCloudExtField *fd, bool *nullable);

/**
 * @brief       Free a Field pointer.
 */
void OhCloudExtFieldFree(OhCloudExtField *db);

/**
 * @brief       Type declaration of Rust cloud extension struct CloudInfo. Can be obtained from
 *              CloudServerGetServiceInfo.
 * @attention   CloudInfo is obtained from CloudSync to provide relevant information to users. When done, users should
 *              call CloudInfoFree to release memory and prevent memory leak.
 */
typedef struct {
    const size_t id;
} OhCloudExtCloudInfo;

/**
 * @brief       Get user from a CloudInfo pointer.
 */
int OhCloudExtCloudInfoGetUser(const OhCloudExtCloudInfo *info, int *user);

/**
 * @brief       Get user from a CloudInfo pointer.
 */
int OhCloudExtCloudInfoGetId(
    const OhCloudExtCloudInfo *info,
    unsigned char **id,
    unsigned int *idLen
);

/**
 * @brief       Get total space from a CloudInfo pointer.
 */
int OhCloudExtCloudInfoGetTotalSpace(
    const OhCloudExtCloudInfo *info,
    unsigned long long *totalSpace
);

/**
 * @brief       Get remain space from a CloudInfo pointer.
 */
int OhCloudExtCloudInfoGetRemainSpace(
    const OhCloudExtCloudInfo *info,
    unsigned long long *remainSpace
);

/**
 * @brief       Check whether a CloudInfo enables cloud sync.
 */
int OhCloudExtCloudInfoEnabled(
    const OhCloudExtCloudInfo *info,
    bool *enableCloud
);

/**
 * @brief       Get hashmap of AppInfo from a CloudInfo pointer.
 * @param       appInfo     [OUT] HashMap<String, AppInfo>, bundle name as key
 */
int OhCloudExtCloudInfoGetAppInfo(
    const OhCloudExtCloudInfo *info,
    OhCloudExtHashMap **appInfo
);

/**
 * @brief       Destroy an CloudInfo instance.
 * @param       ptr     [IN]    The pointer of CloudInfo that should be destroyed.
 */
void OhCloudExtCloudInfoFree(OhCloudExtCloudInfo *ptr);

/**
 * @brief       Type declaration of Rust cloud extension struct AppInfo.
 * @attention   AppInfo is obtained from CloudInfo. When CloudInfo is freed, AppInfo will also be
 *              freed.
 */
typedef struct {
    const size_t id;
} OhCloudExtAppInfo;

/**
 * @brief       Get id from an AppInfo pointer.
 */
int OhCloudExtAppInfoGetAppId(
    const OhCloudExtAppInfo *appInfo,
    unsigned char **appId,
    unsigned int *idLen
);

/**
 * @brief       Get bundle name from an AppInfo pointer.
 */
int OhCloudExtAppInfoGetBundleName(
    const OhCloudExtAppInfo *appInfo,
    unsigned char **bundleName,
    unsigned int *len
);

/**
 * @brief       Check whether an AppInfo pointer allows cloud switch.
 */
int OhCloudExtAppInfoGetCloudSwitch(
    const OhCloudExtAppInfo *appInfo,
    bool *cloudSwitch
);

/**
 * @brief       Get instance id from an AppInfo pointer.
 */
int OhCloudExtAppInfoGetInstanceId(
    const OhCloudExtAppInfo *appInfo,
    int *id
);

/**
 * @brief       Free a AppInfo ptr.
 */
void OhCloudExtAppInfoFree(OhCloudExtAppInfo *info);

/**
 * @brief       Type declaration of Rust cloud extension struct SchemaMeta. Can be obtained from
 *              CloudSyncGetServiceInfo.
 * @attention   SchemaMeta is obtained from CloudSync to provide relevant information to users. When done, users
 *              should call SchemaMetaFree to release memory and prevent memory leak.
 */
typedef struct {
    const size_t id;
} OhCloudExtSchemaMeta;

/**
 * @brief       Get version from a SchemaMeta pointer.
 */
int OhCloudExtSchemaMetaGetVersion(const OhCloudExtSchemaMeta *schema, int *version);

/**
 * @brief       Get bundle name from a SchemaMeta pointer.
 */
int OhCloudExtSchemaMetaGetBundleName(const OhCloudExtSchemaMeta *schema, unsigned char **name, unsigned int *len);

/**
 * @brief       Get databases from a SchemaMeta instance.
 * @param       schema     [IN]
 *              db         [IN] Vec<Database>
 * @attention   The Vector returned should be freed by OhCloudExtVectorFree.
 */
int OhCloudExtSchemaMetaGetDatabases(
    const OhCloudExtSchemaMeta *schema,
    OhCloudExtVector **db
);

/**
 * @brief       Destroy an SchemaMeta instance.
 * @param       ptr     [IN]    The pointer of SchemaMeta that should be destroyed.
 */
void OhCloudExtSchemaMetaFree(OhCloudExtSchemaMeta *ptr);

/**
 * @brief       Type declaration of Rust cloud extension struct RelationSet.
 * @attention   When done, users should call `RelationSetFree` to release memory and prevent memory leak.
 */
typedef struct {
    const size_t id;
} OhCloudExtRelationSet;

/**
 * @brief       Create a RelationSet instance by bundle name, and relations.
 * @param       relations  [IN] HashMap<String, String>, Database alias as key, subscription id and time generated
 *                              by the cloud as value
 * @attention   The hashmap passed in will be managed in the Rust side, so don't call free on this pointer again.
 */
OhCloudExtRelationSet *OhCloudExtRelationSetNew(
    const unsigned char *bundleName,
    unsigned int nameLen,
    int expireTime,
    OhCloudExtHashMap *relations
);

/**
 * @brief       Get bundle name from a RelationSet pointer.
 */
int OhCloudExtRelationSetGetBundleName(const OhCloudExtRelationSet *re, unsigned char **name, unsigned int *len);

/**
 * @brief       Get expire time from a RelationSet pointer.
 */
int OhCloudExtRelationSetGetExpireTime(const OhCloudExtRelationSet *re, unsigned long long *time);

/**
 * @brief       Get relations from a RelationSet pointer.
 * @param       relations  [OUT] HashMap<String, u32>, Database alias as key, subscription id and time generated
 *                               by the cloud as value
 * @attention   The hashmap returned should be freed by OhCloudExtHashMapFree.
 */
int OhCloudExtRelationSetGetRelations(const OhCloudExtRelationSet *re, OhCloudExtHashMap **relations);

/**
 * @brief       Free a RelationSet pointer.
 */
void OhCloudExtRelationSetFree(OhCloudExtRelationSet *ptr);

/**
 * @brief       Type declaration of Rust cloud extension struct CloudDbData. Can be obtained from CloudDbBatchQuery.
 * @attention   CloudDbData is obtained from CloudDatabase to provide relevant information to users. When done, users
 *              should call `CloudDbDataFree` to release memory and prevent memory leak.
 */
typedef struct {
    const size_t id;
} OhCloudExtCloudDbData;

/**
 * @brief       Get the next cursor from a CloudDbData pointer.
 */
int OhCloudExtCloudDbDataGetNextCursor(const OhCloudExtCloudDbData *data, unsigned char **cursor, unsigned int *len);

/**
 * @brief       Check whether a CloudDbData has more data.
 */
int OhCloudExtCloudDbDataGetHasMore(const OhCloudExtCloudDbData *data, bool *hasMore);

/**
 * @brief       Get vector of values from a CloudDbData pointer.
 * @param       values  [OUT] Vec<ValueBucket>
 * @attention   The vector returned should be freed by OhCloudExtVectorFree.
 */
int OhCloudExtCloudDbDataGetValues(const OhCloudExtCloudDbData *data, OhCloudExtVector **values);

/**
 * @brief       Free a CloudDbData pointer.
 */
void OhCloudExtCloudDbDataFree(OhCloudExtCloudDbData *ptr);

/**
 * @brief       Type declaration of Rust cloud extension struct KeyName.
 */
typedef struct {
    unsigned char *key;
    unsigned int keyLen;
} OhCloudExtKeyName;

/**
 * @brief       Create a KeyName instance by key, and keyLen.
 */
OhCloudExtKeyName OhCloudExtKeyNameNew(const unsigned char *key, unsigned int keyLen);

/**
 * @brief       Type declaration of Rust cloud extension struct ValueBucket.
 */
typedef struct {
    const size_t id;
} OhCloudExtValueBucket;

/**
 * @brief      Get ValueBucket all keys from a ValueBucket ptr.
 */
int OhCloudExtValueBucketGetKeys(OhCloudExtValueBucket *info, OhCloudExtVector **keys, unsigned int *keysLen);

/**
 * @brief      Get ValueBucket Value from a ValueBucket prt.
 */
int OhCloudExtValueBucketGetValue(OhCloudExtValueBucket *info, OhCloudExtKeyName keyName,
                                  OhCloudExtValueType *typ, void **content, unsigned int *contentLen);

/**
 * @brief       Free a ValueBucket ptr.
 */
void OhCloudExtValueBucketFree(OhCloudExtValueBucket *info);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // CLOUD_EXTENSION_TYPES_H
