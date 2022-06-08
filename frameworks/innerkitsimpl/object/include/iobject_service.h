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

#ifndef DISTRIBUTEDDATAFWK_IOBJECT_SERVICE_H
#define DISTRIBUTEDDATAFWK_IOBJECT_SERVICE_H

#include <string>

#include <iremote_broker.h>
#include "object_service.h"
#ifndef DECLARE_INTERFACE_DESCRIPTOR_IN
#define DECLARE_INTERFACE_DESCRIPTOR_IN(DESCRIPTOR)                      \
    static inline const std::u16string &GetDescriptor()                  \
    {                                                                    \
        static std::u16string metaDescriptor_ = { DESCRIPTOR };          \
        return metaDescriptor_;                                          \
    }
#endif

namespace OHOS::DistributedObject {
class IObjectService : public ObjectService, public IRemoteBroker {
public:
    enum {
        OBJECTSTORE_SAVE,
        OBJECTSTORE_REVOKE_SAVE,
        OBJECTSTORE_RETRIEVE,
        OBJECTSTORE_SERVICE_CMD_MAX
    };
    DECLARE_INTERFACE_DESCRIPTOR_IN(u"OHOS.DistributedObject.IObjectService");
};
} // namespace OHOS::DistributedRdb
#endif
