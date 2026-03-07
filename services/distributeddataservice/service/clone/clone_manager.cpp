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
#include "clone_manager.h"

#include <fcntl.h>

#include "crypto/crypto_manager.h"
#include "unique_fd.h"
#include "directory/directory_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "securec.h"
#include "utils/anonymous.h"
#include "utils/base64_utils.h"
#include "utils/constant.h"

namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using SecretKeyMeta = DistributedData::SecretKeyMetaData;

constexpr const char *CLONE_KEY_ALIAS = "distributed_db_backup_key";
constexpr int KEY_SIZE = 32;
constexpr int AES_256_NONCE_SIZE = 32;

std::string GetBackupReplyCode(int replyCode, const std::string &info)
{
    CloneReplyCode reply;
    CloneReplyResult result;
    result.errorCode = std::to_string(replyCode);
    result.errorInfo = info;
    reply.resultInfo.push_back(std::move(result));
    return Serializable::Marshall(reply);
}

std::vector<uint8_t> ConvertDecStrToVec(const std::string &inData)
{
    std::vector<uint8_t> outData;
    auto splitedToken = Constant::Split(inData, ",");
    outData.reserve(splitedToken.size());
    for (auto &token : splitedToken) {
        outData.push_back(static_cast<uint8_t>(atoi(token.c_str())));
    }
    return outData;
}

bool ImportCloneKey(const std::string &keyStr)
{
    auto key = ConvertDecStrToVec(keyStr);
    if (key.size() != KEY_SIZE) {
        ZLOGE("ImportKey failed, key size not correct, key size:%{public}zu", key.size());
        key.assign(key.size(), 0);
        return false;
    }

    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    if (!CryptoManager::GetInstance().ImportKey(key, cloneKeyAlias)) {
        key.assign(key.size(), 0);
        return false;
    }
    key.assign(key.size(), 0);
    return true;
}

void DeleteCloneKey()
{
    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    CryptoManager::GetInstance().DeleteKey(cloneKeyAlias);
}

bool WriteBackupInfo(const std::string &content, const std::string &backupPath)
{
    FILE *fp = fopen(backupPath.c_str(), "w");

    if (!fp) {
        ZLOGE("Secret key backup file fopen failed, path: %{public}s, errno: %{public}d",
            Anonymous::Change(backupPath).c_str(), errno);
        return false;
    }
    size_t ret = fwrite(content.c_str(), 1, content.length(), fp);
    if (ret != content.length()) {
        ZLOGE("Secret key backup file fwrite failed, path: %{public}s, errno: %{public}d",
            Anonymous::Change(backupPath).c_str(), errno);
        (void)fclose(fp);
        return false;
    }

    (void)fflush(fp);
    (void)fsync(fileno(fp));
    (void)fclose(fp);
    return true;
}

std::vector<uint8_t> ReEncryptKey(const std::string &key, SecretKeyMetaData &secretKeyMeta,
    const StoreMetaData &metaData, const std::vector<uint8_t> &iv)
{
    if (!MetaDataManager::GetInstance().LoadMeta(key, secretKeyMeta, true)) {
        return {};
    };
    CryptoManager::CryptoParams decryptParams = {
        .area = secretKeyMeta.area, .userId = metaData.user, .nonce = secretKeyMeta.nonce
    };
    auto password = CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptParams);
    if (password.empty()) {
        return {};
    }
    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    CryptoManager::CryptoParams encryptParams = { .keyAlias = cloneKeyAlias, .nonce = iv };
    auto reEncryptedKey = CryptoManager::GetInstance().Encrypt(password, encryptParams);
    password.assign(password.size(), 0);
    if (reEncryptedKey.size() == 0) {
        return {};
    };
    return reEncryptedKey;
}

std::string GetSecretKeyBackup(const std::vector<DistributedData::CloneBundleInfo> &bundleInfos,
    const std::string &userId, const std::vector<uint8_t> &iv, const std::string &localDeviceId)
{
    SecretKeyBackupData backupInfos;
    // std::string deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    for (const auto &bundleInfo : bundleInfos) {
        std::string metaPrefix = StoreMetaData::GetKey({ localDeviceId, userId, "default", bundleInfo.bundleName });
        std::vector<StoreMetaData> StoreMetas;
        if (!MetaDataManager::GetInstance().LoadMeta(metaPrefix, StoreMetas, true)) {
            continue;
        };
        for (const auto &storeMeta : StoreMetas) {
            if (!storeMeta.isEncrypt) {
                continue;
            };
            auto key = storeMeta.GetSecretKey();
            SecretKeyMetaData secretKeyMeta;
            auto reEncryptedKey = ReEncryptKey(key, secretKeyMeta, storeMeta, iv);
            if (reEncryptedKey.size() == 0) {
                continue;
            };
            SecretKeyBackupData::BackupItem item;
            item.time = secretKeyMeta.time;
            item.bundleName = bundleInfo.bundleName;
            item.dbName = storeMeta.storeId;
            item.instanceId = storeMeta.instanceId;
            item.sKey = DistributedData::Base64::Encode(reEncryptedKey);
            item.storeType = secretKeyMeta.storeType;
            item.user = userId;
            item.area = storeMeta.area;
            backupInfos.infos.push_back(std::move(item));
        }
    }
    return Serializable::Marshall(backupInfos);
}

int32_t OnBackup(MessageParcel &data, MessageParcel &reply, const std::string &localDeviceId)
{
    CloneBackupInfo backupInfo;
    if (!backupInfo.Unmarshal(data.ReadString()) || backupInfo.bundleInfos.empty() || backupInfo.userId.empty()) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return -1;
    }

    if (!ImportCloneKey(backupInfo.encryptionInfo.symkey)) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return -1;
    }
    std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');

    auto iv = ConvertDecStrToVec(backupInfo.encryptionInfo.iv);
    if (iv.size() != AES_256_NONCE_SIZE) {
        ZLOGE("Iv size not correct, iv size:%{public}zu", iv.size());
        iv.assign(iv.size(), 0);
        return -1;
    }

    auto content = GetSecretKeyBackup(backupInfo.bundleInfos, backupInfo.userId, iv, localDeviceId);
    DeleteCloneKey();

    std::string backupPath = DirectoryManager::GetInstance().GetClonePath(backupInfo.userId);
    if (backupPath.empty()) {
        ZLOGE("GetClonePath failed, userId:%{public}s errno: %{public}d", backupInfo.userId.c_str(), errno);
        return -1;
    }
    if (!WriteBackupInfo(content, backupPath)) {
        return -1;
    }

    UniqueFd fd(-1);
    fd = UniqueFd(open(backupPath.c_str(), O_RDONLY));
    std::string replyCode = GetBackupReplyCode(0);
    if (!reply.WriteFileDescriptor(fd) || !reply.WriteString(replyCode)) {
        close(fd.Release());
        ZLOGE("OnBackup fail: reply wirte fail, fd:%{public}d", fd.Get());
        return -1;
    }

    close(fd.Release());
    return 0;
}

int32_t ReplyForRestore(MessageParcel &reply, int32_t result)
{
    std::string replyCode = GetBackupReplyCode(result);
    if (!reply.WriteString(replyCode)) {
        return -1;
    }
    return result;
}

bool ParseSecretKeyFile(MessageParcel &data, SecretKeyBackupData &backupData)
{
    UniqueFd fd(data.ReadFileDescriptor());
    struct stat statBuf;
    if (fd.Get() < 0 || fstat(fd.Get(), &statBuf) < 0) {
        ZLOGE("Parse backup secret key failed, fd:%{public}d, errno:%{public}d", fd.Get(), errno);
        return false;
    }
    const off_t sizeMax = 1024 * 1024 * 500;
    if (statBuf.st_size > sizeMax) {
        ZLOGE("Secret key file size exceeds the upper limit, fd:%{public}d, errno:%{public}d", fd.Get(), errno);
        return false;
    }
    std::vector<char> buffer(statBuf.st_size);
    ssize_t fileSize = read(fd.Get(), buffer.data(), statBuf.st_size);
    if (fileSize < 0) {
        ZLOGE("Read backup secret key failed. errno:%{public}d", errno);
        return false;
    }
    std::string keyLoad(buffer.data(), static_cast<size_t>(fileSize));
    memset_s(buffer.data(), buffer.size(), 0, buffer.size());
    DistributedData::Serializable::Unmarshall(keyLoad, backupData);
    std::fill(keyLoad.begin(), keyLoad.end(), '\0');
    return true;
}

bool RestoreSecretKey(
    const SecretKeyBackupData::BackupItem &item, const std::string &userId, const std::vector<uint8_t> &iv)
{
    auto sKey = DistributedData::Base64::Decode(item.sKey);
    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    CryptoManager::CryptoParams decryptParams = { .keyAlias = cloneKeyAlias, .nonce = iv };
    auto rawKey = CryptoManager::GetInstance().Decrypt(sKey, decryptParams);
    if (rawKey.empty()) {
        ZLOGE("Decrypt failed, bundleName:%{public}s, storeName:%{public}s, storeType:%{public}d",
            item.bundleName.c_str(), Anonymous::Change(item.dbName).c_str(), item.storeType);
        sKey.assign(sKey.size(), 0);
        rawKey.assign(rawKey.size(), 0);
        return false;
    }

    StoreMetaData metaData;
    metaData.bundleName = item.bundleName;
    metaData.storeId = item.dbName;
    metaData.user = item.user == "0" ? "0" : userId;
    metaData.instanceId = item.instanceId;

    SecretKeyMetaData secretKey;
    CryptoManager::CryptoParams encryptParams = { .area = item.area, .userId = metaData.user };
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(rawKey, encryptParams);
    if (secretKey.sKey.empty()) {
        ZLOGE("Encrypt failed, bundleName:%{public}s, storeName:%{public}s, storeType:%{public}d",
            item.bundleName.c_str(), Anonymous::Change(item.dbName).c_str(), item.storeType);
        sKey.assign(sKey.size(), 0);
        rawKey.assign(rawKey.size(), 0);
    }
    secretKey.storeType = item.storeType;
    secretKey.nonce = encryptParams.nonce;
    secretKey.area = item.area;
    secretKey.time = { item.time.begin(), item.time.end() };

    sKey.assign(sKey.size(), 0);
    rawKey.assign(rawKey.size(), 0);
    return MetaDataManager::GetInstance().SaveMeta(metaData.GetCloneSecretKey(), secretKey, true);
}

int32_t OnRestore(MessageParcel &data, MessageParcel &reply)
{
    SecretKeyBackupData backupData;
    if (!ParseSecretKeyFile(data, backupData) || backupData.infos.size() == 0) {
        return ReplyForRestore(reply, -1);
    }
    CloneBackupInfo backupInfo;
    bool ret = backupInfo.Unmarshal(data.ReadString());
    if (!ret || backupInfo.userId.empty()) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return ReplyForRestore(reply, -1);
    }

    auto iv = ConvertDecStrToVec(backupInfo.encryptionInfo.iv);
    if (iv.size() != AES_256_NONCE_SIZE) {
        ZLOGE("Iv size not correct, iv size:%{public}zu", iv.size());
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return ReplyForRestore(reply, -1);
    }

    if (!ImportCloneKey(backupInfo.encryptionInfo.symkey)) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        DeleteCloneKey();
        return ReplyForRestore(reply, -1);
    }
    std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');

    for (const auto &item : backupData.infos) {
        if (!item.IsValid() || !RestoreSecretKey(item, backupInfo.userId, iv)) {
            continue;
        }
    }
    DeleteCloneKey();
    return ReplyForRestore(reply, 0);
}
} // namespace OHOS::DistributedKv