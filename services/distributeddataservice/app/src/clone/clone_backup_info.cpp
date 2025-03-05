/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "clone_backup_info.h"

namespace OHOS {
namespace DistributedData {
bool CloneEncryptionInfo::Unmarshal(const json &node)
{
    bool res = GetValue(node, "encryption_symkey", symkey);
    res = GetValue(node, "encryption_algname", algName) && res;
    res = GetValue(node, "gcmParams_iv", iv) && res;
    return res;
}

bool CloneEncryptionInfo::Marshal(json &node) const
{
    return false;
}

CloneEncryptionInfo::~CloneEncryptionInfo()
{
    symkey.assign(symkey.size(), 0);
    algName.assign(algName.size(), 0);
    iv.assign(iv.size(), 0);
}

bool CloneBundleInfo::Unmarshal(const json &node)
{
    bool res = GetValue(node, GET_NAME(bundleName), bundleName);
    res = GetValue(node, GET_NAME(accessTokenId), accessTokenId) && res;
    return res;
}

bool CloneBundleInfo::Marshal(json &node) const
{
    return false;
}

bool CloneBackupInfo::Unmarshal(const json &node)
{
    auto items = DistributedData::Serializable::ToJson(node);
    if (!items.is_array()) {
        return false;
    }
    std::string type;
    auto size = items.size();
    for (size_t i = 0; i < size; i++) {
        bool result = GetValue(items[i], GET_NAME(type), type);
        if (!result || type.empty()) {
            continue;
        }
        if (type == "encryption_info") {
            GetValue(items[i], "detail", encryptionInfo);
        } else if (type == "application_selection") {
            GetValue(items[i], "detail", bundleInfos);
        } else if (type == "userId") {
            GetValue(items[i], "detail", userId);
        }
    }
    return true;
}

bool CloneBackupInfo::Marshal(json &node) const
{
    return false;
}

bool CloneReplyResult::Marshal(json &node) const
{
    SetValue(node[GET_NAME(type)], type);
    SetValue(node[GET_NAME(errorCode)], errorCode);
    SetValue(node[GET_NAME(errorInfo)], errorInfo);
    return true;
}

bool CloneReplyResult::Unmarshal(const json &node)
{
    return false;
}

bool CloneReplyCode::Marshal(json &node) const
{
    SetValue(node[GET_NAME(resultInfo)], resultInfo);
    return true;
}

bool CloneReplyCode::Unmarshal(const json &node)
{
    return false;
}
} // namespace DistributedData
} // namespace OHOS
