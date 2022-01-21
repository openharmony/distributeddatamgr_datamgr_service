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

#define LOG_TAG "RdbParcel"

#include "rdb_parcel.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
RdbStoreParam::RdbStoreParam(const std::string& bundleName, const std::string& directory,
                             const std::string& storeName, int type, bool isAutoSync)
    : bundleName_(bundleName), path_(directory), storeName_(storeName), type_(type),
      isAutoSync_(isAutoSync)
{
}

bool RdbStoreParam::IsValid() const
{
    return !bundleName_.empty() && !path_.empty() && !storeName_.empty();
}

bool RdbStoreParam::Marshalling(MessageParcel& data) const
{
    if (!data.WriteString(bundleName_)) {
        ZLOGE("RdbStoreParam write bundle name failed");
        return false;
    }
    if (!data.WriteString(path_)) {
        ZLOGE("RdbStoreParam write directory failed");
        return false;
    }
    if (!data.WriteString(storeName_)) {
        ZLOGE("RdbStoreParam write store name failed");
        return false;
    }
    if (!data.WriteInt32(type_)) {
        ZLOGE("RdbStoreParam write type failed");
        return false;
    }
    if (!data.WriteBool(isAutoSync_)) {
        ZLOGE("RdbStoreParam write auto sync failed");
        return false;
    }
    return true;
}

bool RdbStoreParam::UnMarshalling(MessageParcel& data)
{
    if (!data.ReadString(bundleName_)) {
        ZLOGE("RdbStoreParam read bundle name failed");
        return false;
    }
    if (!data.ReadString(path_)) {
        ZLOGE("RdbStoreParam read directory failed");
        return false;
    }
    if (!data.ReadString(storeName_)) {
        ZLOGE("RdbStoreParam read store name failed");
        return false;
    }
    if (!data.ReadInt32(type_)) {
        ZLOGE("RdbStoreParam read type failed");
        return false;
    }
    if (!data.ReadBool(isAutoSync_)) {
        ZLOGE("RdbStoreParam read auto sync failed");
        return false;
    }
    return true;
}
}

