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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLONE_CLONE_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLONE_CLONE_MANAGER_H

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#include "clone_backup_info.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "message_parcel.h"
#include "secret_key_backup_data.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::DistributedData;

/**
 * @brief Handle backup request
 * @param data Input message parcel containing backup info
 * @param reply Output message parcel for response
 * @param localDeviceId The local device ID
 * @return 0 if success, -1 if failed
 */
API_EXPORT int32_t OnBackup(MessageParcel &data, MessageParcel &reply, const std::string &localDeviceId) asm(
    "DistributedDataOnBackup");
    
/**
 * @brief Handle restore request
 * @param data Input message parcel containing backup file and info
 * @param reply Output message parcel for response
 * @return 0 if success, -1 if failed
 */
API_EXPORT int32_t OnRestore(MessageParcel &data, MessageParcel &reply) asm("DistributedDataOnRestore");

/**
 * @brief Get backup reply code in JSON format
 * @param replyCode The reply code
 * @param info The error info string (default empty)
 * @return JSON formatted reply code string
 */
std::string GetBackupReplyCode(int replyCode, const std::string &info = "");

/**
 * @brief Convert decimal string to vector of uint8_t
 * @param inData The decimal string (e.g., "1,2,3,4")
 * @return Vector of uint8_t values
 */
std::vector<uint8_t> ConvertDecStrToVec(const std::string &inData);

/**
 * @brief Import clone key for encryption/decryption
 * @param keyStr The key string in decimal format
 * @return true if import succeeded, false otherwise
 */
bool ImportCloneKey(const std::string &keyStr);

/**
 * @brief Delete the imported clone key
 */
void DeleteCloneKey();

/**
 * @brief Write backup info to file
 * @param content The content to write
 * @param backupPath The backup file path
 * @return true if write succeeded, false otherwise
 */
bool WriteBackupInfo(const std::string &content, const std::string &backupPath);

/**
 * @brief Re-encrypt secret key with clone key
 * @param key The metadata key for loading secret key metadata
 * @param secretKeyMeta Output parameter for secret key metadata
 * @param metaData Store metadata containing user info
 * @param iv The initialization vector for encryption
 * @return Re-encrypted key bytes, empty if failed
 */
std::vector<uint8_t> ReEncryptKey(const std::string &key, SecretKeyMetaData &secretKeyMeta,
    const StoreMetaData &metaData, const std::vector<uint8_t> &iv);

/**
 * @brief Get secret key backup for specified bundles
 * @param bundleInfos The bundle information list
 * @param userId The user ID
 * @param iv The initialization vector for encryption
 * @param localDeviceId The local device ID
 * @return Serialized secret key backup data
 */
std::string GetSecretKeyBackup(const std::vector<DistributedData::CloneBundleInfo> &bundleInfos,
    const std::string &userId, const std::vector<uint8_t> &iv, const std::string &localDeviceId);


/**
 * @brief Reply for restore operation
 * @param reply Output message parcel
 * @param result The result code
 * @return 0 if success, -1 if failed
 */
int32_t ReplyForRestore(MessageParcel &reply, int32_t result);

/**
 * @brief Parse secret key backup file
 * @param data Input message parcel containing file descriptor
 * @param backupData Output parameter for parsed backup data
 * @return true if parse succeeded, false otherwise
 */
bool ParseSecretKeyFile(MessageParcel &data, SecretKeyBackupData &backupData);

/**
 * @brief Restore secret key from backup item
 * @param item The backup item to restore
 * @param userId The user ID
 * @param iv The initialization vector for decryption
 * @return true if restore succeeded, false otherwise
 */
bool RestoreSecretKey(
    const SecretKeyBackupData::BackupItem &item, const std::string &userId, const std::vector<uint8_t> &iv);

/**
 * @brief Check if a string contains only numeric characters (0-9)
 * @param str The string to check
 * @return true if the string is non-empty and contains only digits, false otherwise
 */
bool IsNumber(const std::string& str);
} // namespace OHOS::DistributedKv
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLONE_CLONE_MANAGER_H
