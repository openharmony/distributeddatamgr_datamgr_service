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

#ifndef DISTRIBUTEDDATAFWK_RDB_PARCEL_H
#define DISTRIBUTEDDATAFWK_RDB_PARCEL_H

#include <string>
#include <message_parcel.h>

namespace OHOS::DistributedKv {
enum RdbDistributedType {
    RDB_DEVICE_COLLABORATION,
    RDB_DISTRIBUTED_TYPE_MAX
};

struct RdbStoreParam {
public:
    RdbStoreParam() = default;
    RdbStoreParam(const RdbStoreParam& param) = default;
    RdbStoreParam(const std::string& bundleName, const std::string& directory,
                  const std::string& storeName, int type = RDB_DEVICE_COLLABORATION, bool isAutoSync = false);
    bool IsValid() const;
    bool Marshalling(MessageParcel& data) const;
    bool UnMarshalling(MessageParcel& data);
    
    std::string bundleName_;
    std::string path_;
    std::string storeName_;
    int type_ = 0;
    bool isAutoSync_ = false;
};
}
#endif
