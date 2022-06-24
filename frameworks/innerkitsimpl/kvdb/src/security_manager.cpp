/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "security_manager.h"
#include <limits>
#include <random>

#include "file_ex.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
SecurityManager &SecurityManager::GetInstance()
{
    static SecurityManager instance;
    return instance;
}

SecurityManager::DBPassword SecurityManager::GetDBPassword(const StoreId &storeId, const std::string &path,
    bool encrypt)
{
    if (!encrypt) {
        return DBPassword();
    }

    auto secKey = LoadRandomKey(storeId, path);
    if (secKey.empty()) {
        secKey = Random(KEY_SIZE);
        SaveRandomKey(storeId, path, secKey);
    }

    DBPassword password;
    password.SetValue(secKey.data(), secKey.size());
    secKey.assign(secKey.size(), 0);
    return password;
}

void SecurityManager::DelDBPassword(const StoreId &storeId, const std::string &path)
{
    auto keyPath = path + "/key/" + storeId.storeId + ".key";
    StoreUtil::Remove(keyPath);
    keyPath = path + "/key/" + storeId.storeId + ".rekey";
    StoreUtil::Remove(keyPath);
}

std::vector<uint8_t> SecurityManager::Random(int32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (int32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

std::vector<uint8_t> SecurityManager::LoadRandomKey(const StoreId &storeId, const std::string &path)
{
    auto keyPath = path + "/key/" + storeId.storeId + ".key";
    if (!FileExists(keyPath)) {
        return {};
    }

    std::vector<char> content;
    auto loaded = LoadBufferFromFile(keyPath, content);
    if (!loaded) {
        return {};
    }

    if (content.size() < (sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE + 1) {
        return {};
    }

    size_t offset = 0;
    if (content[offset] != char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE)) {
        return {};
    }

    offset++;
    std::vector<uint8_t> date;
    date.assign(content.begin() + offset, content.begin() + (sizeof(time_t) / sizeof(uint8_t)) + offset);
    offset += (sizeof(time_t) / sizeof(uint8_t));
    std::vector<uint8_t> key{content.begin() + offset, content.begin() + KEY_SIZE + offset};
    content.assign(content.size(), 0);
    return key;
}

bool SecurityManager::SaveRandomKey(const StoreId &storeId, const std::string &path, const std::vector<uint8_t> &key)
{
    auto keyPath = path + "/key";
    StoreUtil::InitPath(keyPath);
    keyPath = keyPath + "/" + storeId.storeId + ".key";
    std::vector<char> content;
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    content.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    content.insert(content.end(), date.begin(), date.end());
    content.insert(content.end(), key.begin(), key.end());
    auto ret = SaveBufferToFile(keyPath, content);
    content.assign(content.size(), 0);
    return ret;
}
} // namespace OHOS::DistributedKv