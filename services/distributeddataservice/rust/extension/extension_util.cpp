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

#define LOG_TAG "ExtensionUtil"
#include "extension_util.h"
#include "log_print.h"

namespace OHOS::CloudData {
std::pair<OhCloudExtVector *, size_t> ExtensionUtil::Convert(DBVBuckets &&buckets)
{
    OhCloudExtVector *datas = OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_HASHMAP_VALUE);
    if (datas == nullptr) {
        return { nullptr, 0 };
    }
    size_t datasLen = 0;
    for (auto &bucket : buckets) {
        auto value = Convert(bucket);
        if (value.first == nullptr) {
            return { nullptr, 0 };
        }
        auto status = OhCloudExtVectorPush(datas, value.first, value.second);
        if (status != ERRNO_SUCCESS) {
            return { nullptr, 0 };
        }
        datasLen += 1;
    }
    return { datas, datasLen };
}

std::pair<OhCloudExtHashMap *, size_t> ExtensionUtil::Convert(const DBVBucket &bucket)
{
    OhCloudExtHashMap *values = OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_VALUE);
    if (values == nullptr) {
        return { nullptr, 0 };
    }
    size_t valuesLen = 0;
    for (auto &[col, dbValue] : bucket) {
        void *value = nullptr;
        size_t valueLen = 0;
        if (dbValue.index() == TYPE_INDEX<DBAsset>) {
            DBAsset dbAsset = std::get<DBAsset>(dbValue);
            auto data = Convert(dbAsset);
            OhCloudExtValue *asset = OhCloudExtValueNew(
                OhCloudExtValueType::VALUEINNERTYPE_ASSET, data.first, data.second);
            value = asset;
            valueLen = data.second;
        } else if (dbValue.index() == TYPE_INDEX<DBAssets>) {
            DBAssets dbAssets = std::get<DBAssets>(dbValue);
            auto data = Convert(dbAssets);
            OhCloudExtValue *assets = OhCloudExtValueNew(
                OhCloudExtValueType::VALUEINNERTYPE_ASSETS, data.first, data.second);
            value = assets;
            valueLen = data.second;
        } else {
            auto data = Convert(dbValue);
            value = data.first;
            valueLen = data.second;
        }
        if (value == nullptr) {
            return { nullptr, 0 };
        }
        auto status = OhCloudExtHashMapInsert(values,
            const_cast<void *>(reinterpret_cast<const void *>(col.c_str())), col.size(), value, valueLen);
        if (status != ERRNO_SUCCESS) {
            return { nullptr, 0 };
        }
        valuesLen += valueLen;
    }
    return { values, valuesLen };
}

std::pair<OhCloudExtValue *, size_t> ExtensionUtil::Convert(const DBValue &dbValue)
{
    OhCloudExtValue *value = nullptr;
    size_t valueLen = 0;
    if (dbValue.index() == TYPE_INDEX<int64_t>) {
        int64_t val = std::get<int64_t>(dbValue);
        value = OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_INT, &val, sizeof(int64_t));
        valueLen = sizeof(int64_t);
    } else if (dbValue.index() == TYPE_INDEX<std::string>) {
        std::string val = std::get<std::string>(dbValue);
        value = OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_STRING,
            const_cast<void *>(reinterpret_cast<const void *>(val.c_str())), val.size());
        valueLen = val.size();
    } else if (dbValue.index() == TYPE_INDEX<bool>) {
        bool val = std::get<bool>(dbValue);
        value = OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_BOOL, &val, sizeof(bool));
        valueLen = sizeof(bool);
    } else if (dbValue.index() == TYPE_INDEX<DBBytes>) {
        DBBytes val = std::get<DBBytes>(dbValue);
        if (!val.empty()) {
            value = OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_BYTES, &val[0], val.size());
            valueLen = val.size() * sizeof(uint8_t);
        }
    } else {
        value = OhCloudExtValueNew(OhCloudExtValueType::VALUEINNERTYPE_EMPTY, nullptr, 0);
        valueLen = 0;
    }
    return { value, valueLen };
}

std::pair<OhCloudExtDatabase *, size_t> ExtensionUtil::Convert(const DBMeta &dbMeta)
{
    OhCloudExtHashMap *databases = OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_TABLE);
    if (databases == nullptr) {
        return { nullptr, 0 };
    }
    size_t dbLen = 0;
    for (auto &table : dbMeta.tables) {
        auto [fields, tbLen] = Convert(table);
        if (fields == nullptr) {
            return { nullptr, 0 };
        }
        OhCloudExtTable *tb = OhCloudExtTableNew(reinterpret_cast<const unsigned char *>(table.name.c_str()),
            table.name.size(), reinterpret_cast<const unsigned char *>(table.alias.c_str()),
            table.alias.size(), fields);
        if (tb == nullptr) {
            return { nullptr, 0 };
        }
        tbLen = tbLen + table.name.size() + table.alias.size();
        auto status = OhCloudExtHashMapInsert(databases,
            const_cast<void *>(reinterpret_cast<const void *>(table.name.c_str())), table.name.size(), tb, tbLen);
        if (status != ERRNO_SUCCESS) {
            return { nullptr, 0 };
        }
    }
    dbLen = dbLen + dbMeta.name.size() + dbMeta.alias.size();
    OhCloudExtDatabase *db = OhCloudExtDatabaseNew(reinterpret_cast<const unsigned char *>(dbMeta.name.c_str()),
        dbMeta.name.size(), reinterpret_cast<const unsigned char *>(dbMeta.alias.c_str()),
        dbMeta.alias.size(), databases);
    if (db == nullptr) {
        return { nullptr, 0 };
    }
    return { db, dbLen };
}

std::pair<OhCloudExtVector *, size_t> ExtensionUtil::Convert(const DBTable &dbTable)
{
    size_t tbLen = 0;
    OhCloudExtVector *fields = OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_FIELD);
    if (fields == nullptr) {
        return { nullptr, 0 };
    }
    for (auto &field : dbTable.fields) {
        size_t fdLen =  field.colName.size() + field.alias.size() + sizeof(int) + sizeof(bool) * 2;
        OhCloudExtFieldBuilder builder {
            .colName = reinterpret_cast<const unsigned char *>(field.colName.c_str()),
            .colNameLen = field.colName.size(),
            .alias = reinterpret_cast<const unsigned char *>(field.alias.c_str()),
            .aliasLen = field.alias.size(),
            .typ = static_cast<unsigned int>(field.type),
            .primary = field.primary,
            .nullable = field.nullable
        };
        OhCloudExtField *fd = OhCloudExtFieldNew(&builder);
        if (fd == nullptr) {
            return { nullptr, 0 };
        }
        OhCloudExtVectorPush(fields, fd, fdLen);
        tbLen += fdLen;
    }
    return { fields, tbLen };
}

DBVBuckets ExtensionUtil::ConvertBuckets(OhCloudExtVector *values)
{
    DBVBuckets buckets;
    size_t len = 0;
    auto status = OhCloudExtVectorGetLength(values, reinterpret_cast<unsigned int *>(&len));
    if (status != ERRNO_SUCCESS || len == 0) {
        return buckets;
    }
    buckets.reserve(len);
    for (size_t i = 0; i < len; i++) {
        void *value = nullptr;
        size_t valueLen = 0;
        status = OhCloudExtVectorGet(values, i, &value, reinterpret_cast<unsigned int *>(&valueLen));
        if (status != ERRNO_SUCCESS || value == nullptr) {
            return buckets;
        }
        auto pValues = std::shared_ptr<OhCloudExtHashMap>(reinterpret_cast<OhCloudExtHashMap *>(value),
            [](auto *val) {
                OhCloudExtHashMapFree(val);
            });
        buckets.emplace_back(ConvertBucket(pValues.get()));
    }
    return buckets;
}

DBVBucket ExtensionUtil::ConvertBucket(OhCloudExtHashMap *value)
{
    DBVBucket bucket;
    OhCloudExtVector *valKeys = nullptr;
    OhCloudExtVector *valValues = nullptr;
    auto status = OhCloudExtHashMapIterGetKeyValuePair(value, &valKeys, &valValues);
    if (status != ERRNO_SUCCESS || valKeys == nullptr || valValues == nullptr) {
        return bucket;
    }
    auto pValKeys = std::shared_ptr<OhCloudExtVector>(valKeys, [](auto *valkey) { OhCloudExtVectorFree(valkey); });
    auto pValValues = std::shared_ptr<OhCloudExtVector>(valValues, [](auto *valValue) {
        OhCloudExtVectorFree(valValue);
    });
    size_t valKeysLen = 0;
    size_t valValuesLen = 0;
    OhCloudExtVectorGetLength(pValKeys.get(), reinterpret_cast<unsigned int *>(&valKeysLen));
    OhCloudExtVectorGetLength(pValValues.get(), reinterpret_cast<unsigned int *>(&valValuesLen));
    if (valKeysLen == 0 || valKeysLen != valValuesLen) {
        return bucket;
    }
    for (size_t j = 0; j < valKeysLen; j++) {
        void *keyItem = nullptr;
        size_t keyItemLen = 0;
        status = OhCloudExtVectorGet(pValKeys.get(), j, &keyItem, reinterpret_cast<unsigned int *>(&keyItemLen));
        if (status != ERRNO_SUCCESS || keyItem == nullptr) {
            return bucket;
        }
        void *valueItem = nullptr;
        size_t valueItemLen = 0;
        status = OhCloudExtVectorGet(pValValues.get(), j, &valueItem, reinterpret_cast<unsigned int *>(&valueItemLen));
        if (status != ERRNO_SUCCESS || valueItem == nullptr) {
            return bucket;
        }
        OhCloudExtValue *valueOut = reinterpret_cast<OhCloudExtValue *>(valueItem);
        std::string keyOut(reinterpret_cast<char *>(keyItem), keyItemLen);
        bucket[keyOut] = ConvertValue(valueOut);
        OhCloudExtValueFree(valueOut);
    }
    return bucket;
}

DBValue ExtensionUtil::ConvertValue(OhCloudExtValue *value)
{
    DBValue result;
    OhCloudExtValueType type = OhCloudExtValueType::VALUEINNERTYPE_EMPTY;
    void *content = nullptr;
    size_t ctLen = 0;
    auto status = OhCloudExtValueGetContent(value, &type, &content, reinterpret_cast<unsigned int *>(&ctLen));
    if (status != ERRNO_SUCCESS || content == nullptr) {
        return result;
    }
    DoConvertValue(content, ctLen, type, result);
    return result;
}

DBValue ExtensionUtil::ConvertValues(OhCloudExtValueBucket *bucket, const std::string &key)
{
    DBValue result;
    auto keyStr = const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(key.c_str()));
    OhCloudExtKeyName keyName = OhCloudExtKeyNameNew(keyStr, key.size());
    OhCloudExtValueType type = OhCloudExtValueType::VALUEINNERTYPE_EMPTY;
    void *content = nullptr;
    unsigned int ctLen = 0;
    auto status = OhCloudExtValueBucketGetValue(bucket, keyName, &type, &content, &ctLen);
    if (status != ERRNO_SUCCESS || content == nullptr) {
        return result;
    }
    DoConvertValue(content, ctLen, type, result);
    return result;
}

void ExtensionUtil::DoConvertValue(void *content, unsigned int ctLen, OhCloudExtValueType type, DBValue &dbvalue)
{
    switch (type) {
        case OhCloudExtValueType::VALUEINNERTYPE_EMPTY:
            break;
        case OhCloudExtValueType::VALUEINNERTYPE_INT:
            dbvalue = *reinterpret_cast<int64_t *>(content);
            break;
        case OhCloudExtValueType::VALUEINNERTYPE_BOOL:
            dbvalue = *reinterpret_cast<bool *>(content);
            break;
        case OhCloudExtValueType::VALUEINNERTYPE_STRING:
            dbvalue = std::string(reinterpret_cast<char *>(content), ctLen);
            break;
        case OhCloudExtValueType::VALUEINNERTYPE_BYTES: {
            std::vector<uint8_t> bytes;
            uint8_t *begin = reinterpret_cast<uint8_t *>(content);
            for (size_t i = 0; i < ctLen; i++) {
                uint8_t bt = *begin;
                bytes.emplace_back(bt);
                begin = begin + 1;
            }
            dbvalue = bytes;
            break;
        }
        case OhCloudExtValueType::VALUEINNERTYPE_ASSET: {
            OhCloudExtCloudAsset *asset = reinterpret_cast<OhCloudExtCloudAsset *>(content);
            dbvalue = ConvertAsset(asset);
            OhCloudExtCloudAssetFree(asset);
            break;
        }
        case OhCloudExtValueType::VALUEINNERTYPE_ASSETS: {
            OhCloudExtVector *assets = reinterpret_cast<OhCloudExtVector *>(content);
            dbvalue = ConvertAssets(assets);
            OhCloudExtVectorFree(assets);
            break;
        }
        default:
            break;
    }
}

DBAssets ExtensionUtil::ConvertAssets(OhCloudExtVector *values)
{
    DBAssets result;
    size_t assetsLen = 0;
    auto status = OhCloudExtVectorGetLength(values, reinterpret_cast<unsigned int *>(&assetsLen));
    if (status != ERRNO_SUCCESS || assetsLen == 0) {
        return result;
    }
    result.reserve(assetsLen);
    for (size_t i = 0; i < assetsLen; i++) {
        void *value = nullptr;
        size_t valueLen = 0;
        status = OhCloudExtVectorGet(values, i, &value, reinterpret_cast<unsigned int *>(&valueLen));
        if (status != ERRNO_SUCCESS || value == nullptr) {
            return result;
        }
        OhCloudExtCloudAsset *asset = reinterpret_cast<OhCloudExtCloudAsset *>(value);
        result.push_back(ConvertAsset(asset));
        OhCloudExtCloudAssetFree(asset);
    }
    return result;
}

DBAsset ExtensionUtil::ConvertAsset(OhCloudExtCloudAsset *asset)
{
    DBAsset result;
    unsigned char *id = nullptr;
    size_t idLen = 0;
    auto status = OhCloudExtCloudAssetGetId(asset, &id, reinterpret_cast<unsigned int *>(&idLen));
    if (status != ERRNO_SUCCESS || id == nullptr) {
        return result;
    }
    result.id = std::string(reinterpret_cast<char *>(id), idLen);
    unsigned char *name = nullptr;
    size_t nameLen = 0;
    status = OhCloudExtCloudAssetGetName(asset, &name, reinterpret_cast<unsigned int *>(&nameLen));
    if (status != ERRNO_SUCCESS || name == nullptr) {
        return result;
    }
    result.name = std::string(reinterpret_cast<char *>(name), nameLen);
    unsigned char *uri = nullptr;
    size_t uriLen = 0;
    status = OhCloudExtCloudAssetGetUri(asset, &uri, reinterpret_cast<unsigned int *>(&uriLen));
    if (status != ERRNO_SUCCESS || uri == nullptr) {
        return result;
    }
    result.uri = std::string(reinterpret_cast<char *>(uri), uriLen);
    unsigned char *crtTime = nullptr;
    size_t crtTimeLen = 0;
    status = OhCloudExtCloudAssetGetCreateTime(asset, &crtTime, reinterpret_cast<unsigned int *>(&crtTimeLen));
    if (status != ERRNO_SUCCESS || crtTime == nullptr) {
        return result;
    }
    result.createTime = std::string(reinterpret_cast<char *>(crtTime), crtTimeLen);
    ConvertAssetLeft(asset, result);
    return result;
}

void ExtensionUtil::ConvertAssetLeft(OhCloudExtCloudAsset *asset, DBAsset &dbAsset)
{
    unsigned char *mdTime = nullptr;
    size_t mdTimeLen = 0;
    auto status = OhCloudExtCloudAssetGetModifiedTime(asset, &mdTime, reinterpret_cast<unsigned int *>(&mdTimeLen));
    if (status != ERRNO_SUCCESS || mdTime == nullptr) {
        return;
    }
    dbAsset.modifyTime = std::string(reinterpret_cast<char *>(mdTime), mdTimeLen);
    unsigned char *size = nullptr;
    size_t sizeLen = 0;
    status = OhCloudExtCloudAssetGetSize(asset, &size, reinterpret_cast<unsigned int *>(&sizeLen));
    if (status != ERRNO_SUCCESS || size == nullptr) {
        return;
    }
    dbAsset.size = std::string(reinterpret_cast<char *>(size), sizeLen);
    unsigned char *hash = nullptr;
    size_t hashLen = 0;
    status = OhCloudExtCloudAssetGetHash(asset, &hash, reinterpret_cast<unsigned int *>(&hashLen));
    if (status != ERRNO_SUCCESS || hash == nullptr) {
        return;
    }
    dbAsset.hash = std::string(reinterpret_cast<char *>(hash), hashLen);
    unsigned char *path = nullptr;
    size_t pathLen = 0;
    status = OhCloudExtCloudAssetGetLocalPath(asset, &path, reinterpret_cast<unsigned int *>(&pathLen));
    if (status != ERRNO_SUCCESS || path == nullptr) {
        return;
    }
    dbAsset.path = std::string(reinterpret_cast<char *>(path), pathLen);
}

DBInfo ExtensionUtil::ConvertAppInfo(OhCloudExtAppInfo *appInfo)
{
    DBInfo info;
    unsigned char *appId = nullptr;
    size_t appIdLen = 0;
    auto status = OhCloudExtAppInfoGetAppId(appInfo, &appId, reinterpret_cast<unsigned int *>(&appIdLen));
    if (status != ERRNO_SUCCESS || appId == nullptr) {
        return info;
    }
    info.appId = std::string(reinterpret_cast<char *>(appId), appIdLen);
    unsigned char *bundle = nullptr;
    size_t bundleLen = 0;
    status = OhCloudExtAppInfoGetBundleName(appInfo, &bundle, reinterpret_cast<unsigned int *>(&bundleLen));
    if (status != ERRNO_SUCCESS || bundle == nullptr) {
        return info;
    }
    info.bundleName = std::string(reinterpret_cast<char *>(bundle), bundleLen);
    OhCloudExtAppInfoGetCloudSwitch(appInfo, &info.cloudSwitch);
    OhCloudExtAppInfoGetInstanceId(appInfo, &info.instanceId);
    return info;
}

std::pair<OhCloudExtCloudAsset *, size_t> ExtensionUtil::Convert(const DBAsset &dbAsset)
{
    OhCloudExtCloudAssetBuilder builder {
        .version = dbAsset.version,
        .status = ConvertAssetStatus(static_cast<DBAssetStatus>(dbAsset.status)),
        .expiresTime = dbAsset.expiresTime,
        .id = reinterpret_cast<const unsigned char *>(dbAsset.id.c_str()),
        .idLen = dbAsset.id.size(),
        .name = reinterpret_cast<const unsigned char *>(dbAsset.name.c_str()),
        .nameLen = dbAsset.name.size(),
        .uri = reinterpret_cast<const unsigned char *>(dbAsset.uri.c_str()),
        .uriLen = dbAsset.uri.size(),
        .localPath = reinterpret_cast<const unsigned char *>(dbAsset.path.c_str()),
        .localPathLen = dbAsset.path.size(),
        .createTime = reinterpret_cast<const unsigned char *>(dbAsset.createTime.c_str()),
        .createTimeLen = dbAsset.createTime.size(),
        .modifyTime = reinterpret_cast<const unsigned char *>(dbAsset.modifyTime.c_str()),
        .modifyTimeLen = dbAsset.modifyTime.size(),
        .size = reinterpret_cast<const unsigned char *>(dbAsset.size.c_str()),
        .sizeLen = dbAsset.size.size(),
        .hash = reinterpret_cast<const unsigned char *>(dbAsset.hash.c_str()),
        .hashLen = dbAsset.hash.size()
    };
    OhCloudExtCloudAsset *asset = OhCloudExtCloudAssetNew(&builder);
    size_t assetLen = sizeof(uint64_t) * 2 + sizeof(OhCloudExtAssetStatus) + dbAsset.id.size()
        + dbAsset.name.size() + dbAsset.uri.size() + dbAsset.createTime.size() + dbAsset.modifyTime.size()
        + dbAsset.size.size() + dbAsset.hash.size() + dbAsset.path.size();
    if (asset == nullptr) {
        return { nullptr, 0 };
    }
    return { asset, assetLen };
}

std::pair<OhCloudExtVector *, size_t> ExtensionUtil::Convert(const DBAssets &dbAssets)
{
    OhCloudExtVector *assets = OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_CLOUD_ASSET);
    if (assets == nullptr) {
        return { nullptr, 0 };
    }
    size_t assetsLen = 0;
    for (const auto &dbAsset : dbAssets) {
        auto data = Convert(dbAsset);
        if (data.first == nullptr) {
            return { nullptr, 0 };
        }
        auto status = OhCloudExtVectorPush(assets, data.first, data.second);
        if (status != ERRNO_SUCCESS) {
            return { nullptr, 0 };
        }
        assetsLen += 1;
    }
    return { assets, assetsLen };
}

OhCloudExtAssetStatus ExtensionUtil::ConvertAssetStatus(DBAssetStatus status)
{
    switch (status) {
        case DBAssetStatus::STATUS_UNKNOWN:
            return OhCloudExtAssetStatus::ASSETSTATUS_UNKNOWN;
        case DBAssetStatus::STATUS_NORMAL:
            return OhCloudExtAssetStatus::ASSETSTATUS_NORMAL;
        case DBAssetStatus::STATUS_INSERT:
            return OhCloudExtAssetStatus::ASSETSTATUS_INSERT;
        case DBAssetStatus::STATUS_UPDATE:
            return OhCloudExtAssetStatus::ASSETSTATUS_UPDATE;
        case DBAssetStatus::STATUS_DELETE:
            return OhCloudExtAssetStatus::ASSETSTATUS_DELETE;
        case DBAssetStatus::STATUS_ABNORMAL:
            return OhCloudExtAssetStatus::ASSETSTATUS_ABNORMAL;
        case DBAssetStatus::STATUS_DOWNLOADING:
            return OhCloudExtAssetStatus::ASSETSTATUS_DOWNLOADING;
        case DBAssetStatus::STATUS_BUTT:
            return OhCloudExtAssetStatus::ASSETSTATUS_BUTT;
        default:
            ZLOGI("err: 0x%{public}x", status);
            break;
    }
    return OhCloudExtAssetStatus::ASSETSTATUS_UNKNOWN;
}

DBErr ExtensionUtil::ConvertStatus(int status)
{
    switch (status) {
        case ERRNO_SUCCESS:
            return DBErr::E_OK;
        case ERRNO_NETWORK_ERROR:
            return DBErr::E_NETWORK_ERROR;
        case ERRNO_LOCKED_BY_OTHERS:
            return DBErr::E_LOCKED_BY_OTHERS;
        case ERRNO_RECORD_LIMIT_EXCEEDED:
            return DBErr::E_RECODE_LIMIT_EXCEEDED;
        case ERRNO_NO_SPACE_FOR_ASSET:
            return DBErr::E_NO_SPACE_FOR_ASSET;
        case ERRNO_UNSUPPORTED:
            return DBErr::E_NOT_SUPPORT;
        default:
            ZLOGI("err: 0x%{public}x", status);
            break;
    }
    return DBErr::E_ERROR;
}
} // namespace OHOS::CloudData