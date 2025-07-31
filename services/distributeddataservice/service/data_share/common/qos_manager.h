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

#ifndef DATASHARESERVICE_QOS_MANAGER_H
#define DATASHARESERVICE_QOS_MANAGER_H

#include "qos.h"
namespace OHOS {
namespace DataShare {

class QosManager {
public:
    QosManager()
    {
#ifndef IS_EMULATOR
        // set thread qos QOS_USER_INTERACTIVE
        QOS::SetThreadQos(QOS::QosLevel::QOS_USER_INTERACTIVE);
#endif
    }
    ~QosManager()
    {
#ifndef IS_EMULATOR
        QOS::ResetThreadQos();
#endif
    }
};

} // namespace DataShare
} // namespace OHOS
#endif // DATASHARESERVICE_QOS_MANAGER_H