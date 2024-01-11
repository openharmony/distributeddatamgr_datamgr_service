/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_VALUE_UTIL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_VALUE_UTIL_H

#include "cloud_service.h"
#include "cloud_types.h"
#include "cloud/sharing_center.h"

namespace OHOS::CloudData {
namespace SharingUtil {
using SharingPtpant = OHOS::DistributedData::SharingCenter::Participant;
using SharingPlege = OHOS::DistributedData::SharingCenter::Privilege;
using SharingCfm = OHOS::DistributedData::SharingCenter::Confirmation;
using SharingRole = OHOS::DistributedData::SharingCenter::Role;
using SharingResults = OHOS::DistributedData::SharingCenter::QueryResults;
using CenterCode = OHOS::DistributedData::SharingCenter::SharingCode;
using Status = CloudService::Status;

template<typename T, typename O>
std::vector<O> Convert(const std::vector<T> &data);

SharingPlege Convert(const Privilege &privilege);
SharingPtpant Convert(const Participant &participant);
SharingCfm Convert(const Confirmation &cfm);

Privilege Convert(const SharingPlege &privilege);
Participant Convert(const SharingPtpant &participant);
Confirmation Convert(const SharingCfm &cfm);

QueryResults Convert(const SharingResults &results);

std::vector<SharingPtpant> Convert(const std::vector<Participant> &participants);

std::vector<Participant> Convert(const std::vector<SharingPtpant> &input);

Status Convert(CenterCode code);
} // namespace SharingUtil
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_VALUE_UTIL_H
