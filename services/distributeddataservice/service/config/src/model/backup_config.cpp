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

#include "model/backup_config.h"
namespace OHOS {
namespace DistributedData {
bool BackupConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(rules)], rules);
    SetValue(node[GET_NAME(schedularDelay)], schedularDelay);
    SetValue(node[GET_NAME(schedularInternal)], schedularInternal);
    SetValue(node[GET_NAME(backupInternal)], backupInternal);
    SetValue(node[GET_NAME(backupNumber)], backupNumber);
    return true;
}

bool BackupConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(rules), rules);
    GetValue(node, GET_NAME(schedularDelay), schedularDelay);
    GetValue(node, GET_NAME(schedularInternal), schedularInternal);
    GetValue(node, GET_NAME(backupInternal), backupInternal);
    GetValue(node, GET_NAME(backupNumber), backupNumber);
    return true;
}
} // namespace DistributedData
} // namespace OHOS
