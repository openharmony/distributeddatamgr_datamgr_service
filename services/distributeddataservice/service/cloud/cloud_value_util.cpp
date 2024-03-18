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

#include "cloud_value_util.h"

namespace OHOS::CloudData {
namespace SharingUtil {
SharingPlege Convert(const Privilege &privilege)
{
    SharingPlege result;
    result.writable = privilege.writable;
    result.readable = privilege.readable ;
    result.creatable = privilege.creatable;
    result.deletable = privilege.deletable;
    result.shareable = privilege.shareable;
    return result;
}

SharingPtpant Convert(const Participant &participant)
{
    SharingPtpant result;
    result.identity = participant.identity;
    result.role = participant.role;
    result.state = participant.state;
    result.privilege = Convert(participant.privilege);
    result.attachInfo = participant.attachInfo;
    return result;
}

SharingCfm Convert(const Confirmation &cfm)
{
    switch (cfm) {
        case Confirmation::CFM_UNKNOWN:
            return SharingCfm::CFM_UNKNOWN;
        case Confirmation::CFM_ACCEPTED:
            return SharingCfm::CFM_ACCEPTED;
        case Confirmation::CFM_REJECTED:
            return SharingCfm::CFM_REJECTED;
        case Confirmation::CFM_SUSPENDED:
            return SharingCfm::CFM_SUSPENDED;
        case Confirmation::CFM_UNAVAILABLE:
            return SharingCfm::CFM_UNAVAILABLE;
        default:
            break;
    }
    return SharingCfm::CFM_UNKNOWN;
}

Participant Convert(const SharingPtpant &participant)
{
    Participant result;
    result.identity = participant.identity;
    result.role = participant.role;
    result.state = participant.state;
    result.privilege = Convert(participant.privilege);
    result.attachInfo = participant.attachInfo;
    return result;
}

Confirmation Convert(const SharingCfm &cfm)
{
    switch (cfm) {
        case SharingCfm::CFM_UNKNOWN:
            return Confirmation::CFM_UNKNOWN;
        case SharingCfm::CFM_ACCEPTED:
            return Confirmation::CFM_ACCEPTED;
        case SharingCfm::CFM_REJECTED:
            return Confirmation::CFM_REJECTED;
        case SharingCfm::CFM_SUSPENDED:
            return Confirmation::CFM_SUSPENDED;
        case SharingCfm::CFM_UNAVAILABLE:
            return Confirmation::CFM_UNAVAILABLE;
        default:
            break;
    }
    return Confirmation::CFM_UNKNOWN;
}

Privilege Convert(const SharingPlege &privilege)
{
    Privilege result;
    result.writable = privilege.writable;
    result.readable = privilege.readable ;
    result.creatable = privilege.creatable;
    result.deletable = privilege.deletable;
    result.shareable = privilege.shareable;
    return result;
}

QueryResults Convert(const SharingResults &results)
{
    return { std::get<0>(results), std::get<1>(results),
        Convert<SharingPtpant, Participant>(std::get<2>(results)) };
}

std::vector<SharingPtpant> Convert(const std::vector<Participant> &participants)
{
    return Convert<Participant, SharingPtpant>(participants);
}

std::vector<Participant> Convert(const std::vector<SharingPtpant> &input)
{
    return Convert<SharingPtpant, Participant>(input);
}

template<typename T, typename O>
std::vector<O> Convert(const std::vector<T> &data)
{
    std::vector<O> out;
    out.reserve(data.size());
    for (const auto &v : data) {
        out.emplace_back(Convert(v));
    }
    return out;
}

Status Convert(CenterCode code)
{
    switch (code) {
        case CenterCode::IPC_ERROR:
            return Status::IPC_ERROR;
        default:
            break;
    }
    return Status::SUCCESS;
}
} // namespace::SharingUtil
} // namespace OHOS::CloudData
