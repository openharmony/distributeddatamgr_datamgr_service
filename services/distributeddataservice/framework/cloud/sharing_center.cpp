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

#include "cloud/sharing_center.h"
namespace OHOS::DistributedData {
SharingCenter::Results SharingCenter::Share(int32_t userId, const std::string &bundleName,
    const std::string &sharingRes, const Participants &participants)
{
    return {};
}

SharingCenter::Results SharingCenter::Unshare(int32_t userId, const std::string &bundleName,
    const std::string &sharingRes, const Participants &participants)
{
    return {};
}

std::pair<int32_t, std::string> SharingCenter::Exit(int32_t userId, const std::string &bundleName,
    const std::string &sharingRes)
{
    return {};
}

SharingCenter::Results SharingCenter::ChangePrivilege(int32_t userId, const std::string &bundleName,
    const std::string &sharingRes, const Participants &participants)
{
    return {};
}

SharingCenter::QueryResults SharingCenter::Query(int32_t userId, const std::string &bundleName,
    const std::string &sharingRes)
{
    return {};
}

SharingCenter::QueryResults SharingCenter::QueryByInvitation(int32_t userId, const std::string &bundleName,
    const std::string &invitation)
{
    return {};
}

std::tuple<int32_t, std::string, std::string> SharingCenter::ConfirmInvitation(int32_t userId,
    const std::string &bundleName, const std::string &invitation, int32_t confirmation)
{
    return {};
}

std::pair<int32_t, std::string> SharingCenter::ChangeConfirmation(int32_t userId,
    const std::string &bundleName, const std::string &sharingRes, int32_t confirmation)
{
    return {};
}
} // namespace OHOS::DistributedData