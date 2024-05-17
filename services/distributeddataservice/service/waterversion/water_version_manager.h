/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_WATER_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_WATER_MANAGER_H
#include <map>
#include <set>
#include <string>
#include "serializable/serializable.h"
#include "lru_bucket.h"

namespace OHOS {
namespace DistributedData {
class WaterVersionManager {
public:
    enum Type {
        DYNAMIC,
        STATIC,
        BUTT
    };
    class WaterVersionMetaData final : public Serializable {
    public:
        static constexpr uint32_t DEFAULT_VERSION = 0;
        std::string deviceId;
        uint32_t version = DEFAULT_VERSION;
        uint64_t waterVersion = 0;
        Type type;
        std::vector<std::string> keys;
        std::vector<std::vector<uint64_t>> infos;
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        bool IsValid();
        std::string ToAnonymousString() const;
        std::string GetKey() const;
        uint64_t GetVersion();
        static std::string GetPrefix();

    private:
        static constexpr const char *KEY_PREFIX = "WaterVersionMeta";
    };
    API_EXPORT static WaterVersionManager &GetInstance();
    API_EXPORT void Init();
    std::string GenerateWaterVersion(const std::string &bundleName, const std::string &storeName);
    std::string GetWaterVersion(const std::string &bundleName, const std::string &storeName);
    std::pair<bool, uint64_t> GetVersion(const std::string &deviceId, Type type);
    std::string GetWaterVersion(const std::string &deviceId, Type type);
    bool SetWaterVersion(const std::string &bundleName, const std::string &storeName,
        const std::string &waterVersion);
    bool DelWaterVersion(const std::string &deviceId);

private:
    WaterVersionManager();
    ~WaterVersionManager();
    WaterVersionManager(const WaterVersionManager &) = delete;
    WaterVersionManager(WaterVersionManager &&) noexcept = delete;
    WaterVersionManager& operator=(const WaterVersionManager &) = delete;
    WaterVersionManager& operator=(WaterVersionManager &&) = delete;
    static constexpr size_t MAX_DEVICES = 16;
    class WaterVersion {
    public:
        WaterVersion() = default;
        void SetType(Type type);
        void AddKey(const std::string &key);
        bool InitWaterVersion(const WaterVersionMetaData &metaData);
        std::pair<bool, WaterVersionMetaData> GenerateWaterVersion(const std::string &key);
        std::pair<bool, WaterVersionMetaData> GetWaterVersion(const std::string &deviceId);
        bool SetWaterVersion(const std::string &key, const WaterVersionMetaData &metaData);
        bool DelWaterVersion(const std::string &deviceId);
    private:
        static constexpr uint16_t INVALID_VERSION = 0xFFF;
        Type type_;
        std::mutex mutex_;
        std::vector<std::string> keys_;
        LRUBucket<std::string, WaterVersionMetaData> versions_{ MAX_DEVICES };
    };
    static bool InitMeta(WaterVersionMetaData &metaData);
    static WaterVersionMetaData Upgrade(const std::vector<std::string> &keys, const WaterVersionMetaData& meta);
    static std::string Merge(const std::string &bundleName, const std::string &storeName);
    static std::pair<std::string, std::string> Split(const std::string &key);
    static void UpdateWaterVersion(WaterVersionMetaData &metaData);
    static void SaveMatrix(const WaterVersionMetaData &metaData);
    std::vector<WaterVersion> waterVersions_;
};
} // namespace DistributedData
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_WATER_MANAGER_H
