/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_META_DATA_SAVER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_META_DATA_SAVER_H

#include <memory>
#include <string>
#include <vector>

#include "metadata/meta_data_manager.h"
#include "serializable/serializable.h"
#include "visibility.h"

namespace OHOS::DistributedData {

/**
 * @brief RAII-style batch metadata saver.
 *
 * Collects multiple metadata entries and saves them in a single batch operation.
 * Automatically flushes on destruction.
 *
 * @warning NOT THREAD-SAFE
 * This class is not thread-safe and must not be accessed concurrently from
 * multiple threads. Each instance should be used by a single thread only.
 */
class API_EXPORT MetaDataSaver {
public:
    /**
     * @brief Constructor
     * @param isLocal Whether to save to local table (true) or sync table (false)
     */
    explicit MetaDataSaver(bool isLocal = true);

    /**
     * @brief Destructor - automatically flushes if not already done
     */
    ~MetaDataSaver();

    // Disable copy and move
    MetaDataSaver(const MetaDataSaver&) = delete;
    MetaDataSaver& operator=(const MetaDataSaver&) = delete;
    MetaDataSaver(MetaDataSaver&&) = delete;
    MetaDataSaver& operator=(MetaDataSaver&&) = delete;

    /**
     * @brief Add an entry to be saved (automatically serializes the value)
     * @tparam T Type of value to serialize (must be compatible with Serializable::Marshall)
     * @param key The metadata key
     * @param value The value to serialize and save
     */
    template<typename T>
    void Add(const std::string &key, const T &value) {
        entries_.push_back({key, Serializable::Marshall(value)});
    }

    /**
     * @brief Add an already-serialized entry
     * @param key The metadata key
     * @param value The serialized value
     */
    void Add(const std::string &key, const std::string &value);

    /**
     * @brief Get the number of entries currently collected
     * @return Number of entries
     */
    size_t Size() const;

    /**
     * @brief Clear all entries without saving
     */
    void Clear();

private:
    std::vector<MetaDataManager::Entry> entries_;
    bool async_;  // true for local table, false for sync table
};

} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_META_DATA_SAVER_H
