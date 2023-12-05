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

#include "rdb_cloud_data_translate.h"

#include "utils/endian_converter.h"
#include "value_proxy.h"

namespace OHOS::DistributedRdb {
using Asset = DistributedDB::Asset;
using Assets = DistributedDB::Assets;
using DataAsset = NativeRdb::ValueObject::Asset;
using DataAssets = NativeRdb::ValueObject::Assets;
using  ValueProxy = DistributedData::ValueProxy;
std::vector<uint8_t> RdbCloudDataTranslate::AssetToBlob(const Asset &asset)
{
    std::vector<uint8_t> rawData;
    Asset dbAsset = asset;
    dbAsset.flag = static_cast<uint32_t>(DistributedDB::AssetOpType::NO_CHANGE);
    DataAsset dataAsset = ValueProxy::TempAsset(std::move(dbAsset));
    InnerAsset innerAsset(dataAsset);
    auto data = Serializable::Marshall(innerAsset);
    auto size = DistributedData::HostToNet((uint16_t)data.length());
    auto leMagic = DistributedData::HostToNet(ASSET_MAGIC);
    auto magicU8 = reinterpret_cast<uint8_t *>(const_cast<uint32_t *>(&leMagic));
    rawData.insert(rawData.end(), magicU8, magicU8 + sizeof(ASSET_MAGIC));
    rawData.insert(rawData.end(), reinterpret_cast<uint8_t *>(&size),
        reinterpret_cast<uint8_t *>(&size) + sizeof(size));
    rawData.insert(rawData.end(), data.begin(), data.end());
    return rawData;
}

std::vector<uint8_t> RdbCloudDataTranslate::AssetsToBlob(const Assets &assets)
{
    std::vector<uint8_t> rawData;
    auto num = DistributedData::HostToNet((uint16_t)assets.size());
    auto leMagic = DistributedData::HostToNet(ASSETS_MAGIC);
    auto magicU8 = reinterpret_cast<uint8_t *>(const_cast<uint32_t *>(&leMagic));
    rawData.insert(rawData.end(), magicU8, magicU8 + sizeof(ASSETS_MAGIC));
    rawData.insert(rawData.end(), reinterpret_cast<uint8_t *>(&num), reinterpret_cast<uint8_t *>(&num) + sizeof(num));
    for (auto &asset : assets) {
        auto data = AssetToBlob(asset);
        rawData.insert(rawData.end(), data.begin(), data.end());
    }
    return rawData;
}

Asset RdbCloudDataTranslate::BlobToAsset(const std::vector<uint8_t> &blob)
{
    DataAsset asset;
    if (ParserRawData(blob.data(), blob.size(), asset) == 0) {
        return {};
    }
    return ValueProxy::Convert(asset);
}

Assets RdbCloudDataTranslate::BlobToAssets(const std::vector<uint8_t> &blob)
{
    DataAssets assets;
    if (ParserRawData(blob.data(), blob.size(), assets) == 0) {
        return {};
    }
    return ValueProxy::Convert(assets);
}

size_t RdbCloudDataTranslate::ParserRawData(const uint8_t *data, size_t length, DataAsset &asset)
{
    size_t used = 0;
    uint16_t size = 0;

    if (sizeof(ASSET_MAGIC) > length - used) {
        return 0;
    }
    std::vector<uint8_t> alignData;
    alignData.assign(data, data + sizeof(ASSET_MAGIC));
    used += sizeof(ASSET_MAGIC);
    auto hostMagicWord = DistributedData::NetToHost(*(reinterpret_cast<decltype(&ASSET_MAGIC)>(alignData.data())));
    if (hostMagicWord != ASSET_MAGIC) {
        return 0;
    }
    if (sizeof(size) > length - used) {
        return 0;
    }
    alignData.assign(data + used, data + used + sizeof(size));
    used += sizeof(size);
    size = DistributedData::NetToHost(*(reinterpret_cast<decltype(&size)>(alignData.data())));
    if (size > length - used) {
        return 0;
    }
    auto rawData = std::string(reinterpret_cast<const char *>(&data[used]), size);
    InnerAsset innerAsset(asset);
    if (!innerAsset.Unmarshall(rawData)) {
        return 0;
    }
    used += size;
    return used;
}

size_t RdbCloudDataTranslate::ParserRawData(const uint8_t *data, size_t length, DataAssets &assets)
{
    size_t used = 0;
    uint16_t num = 0;

    if (sizeof(ASSETS_MAGIC) > length - used) {
        return 0;
    }
    std::vector<uint8_t> alignData;
    alignData.assign(data, data + sizeof(ASSETS_MAGIC));
    used += sizeof(ASSETS_MAGIC);
    auto hostMagicWord = DistributedData::NetToHost(*(reinterpret_cast<decltype(&ASSETS_MAGIC)>(alignData.data())));
    if (hostMagicWord != ASSETS_MAGIC) {
        return 0;
    }

    if (sizeof(num) > length - used) {
        return 0;
    }
    alignData.assign(data, data + sizeof(num));
    num = DistributedData::NetToHost(*(reinterpret_cast<decltype(&num)>(alignData.data())));
    used += sizeof(num);
    uint16_t count = 0;
    while (used < length && count < num) {
        DataAsset asset;
        auto dataLen = ParserRawData(&data[used], length - used, asset);
        if (dataLen == 0) {
            break;
        }
        used += dataLen;
        assets.push_back(ValueProxy::Convert(asset));
        count++;
    }
    return used;
}

bool RdbCloudDataTranslate::InnerAsset::Marshal(OHOS::DistributedData::Serializable::json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(version)], asset_.version) && ret;
    ret = SetValue(node[GET_NAME(status)], asset_.status) && ret;
    ret = SetValue(node[GET_NAME(expiresTime)], asset_.expiresTime) && ret;
    ret = SetValue(node[GET_NAME(id)], asset_.id) && ret;
    ret = SetValue(node[GET_NAME(name)], asset_.name) && ret;
    ret = SetValue(node[GET_NAME(uri)], asset_.uri) && ret;
    ret = SetValue(node[GET_NAME(path)], asset_.path) && ret;
    ret = SetValue(node[GET_NAME(createTime)], asset_.createTime) && ret;
    ret = SetValue(node[GET_NAME(modifyTime)], asset_.modifyTime) && ret;
    ret = SetValue(node[GET_NAME(size)], asset_.size) && ret;
    ret = SetValue(node[GET_NAME(hash)], asset_.hash) && ret;
    return ret;
}

bool RdbCloudDataTranslate::InnerAsset::Unmarshal(const OHOS::DistributedData::Serializable::json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(version), asset_.version) && ret;
    ret = GetValue(node, GET_NAME(status), asset_.status) && ret;
    ret = GetValue(node, GET_NAME(expiresTime), asset_.expiresTime) && ret;
    ret = GetValue(node, GET_NAME(id), asset_.id) && ret;
    ret = GetValue(node, GET_NAME(name), asset_.name) && ret;
    ret = GetValue(node, GET_NAME(uri), asset_.uri) && ret;
    ret = GetValue(node, GET_NAME(path), asset_.path) && ret;
    ret = GetValue(node, GET_NAME(createTime), asset_.createTime) && ret;
    ret = GetValue(node, GET_NAME(modifyTime), asset_.modifyTime) && ret;
    ret = GetValue(node, GET_NAME(size), asset_.size) && ret;
    ret = GetValue(node, GET_NAME(hash), asset_.hash) && ret;
    return ret;
}
} // namespace OHOS::DistributedRdb
