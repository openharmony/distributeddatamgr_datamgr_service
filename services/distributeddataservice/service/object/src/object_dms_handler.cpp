/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "ObjectDmsHandler"

#include "object_dms_handler.h"

#include "device_manager_adapter.h"
#include "dms_handler.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedObject {
constexpr const char *PKG_NAME = "ohos.distributeddata.service";
void DmsEventListener::DSchedEventNotify(DistributedSchedule::EventNotify &notify)
{
    ObjectDmsHandler::GetInstance().ReceiveDmsEvent(notify);
}

void ObjectDmsHandler::ReceiveDmsEvent(DistributedSchedule::EventNotify &event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    dmsEvents_.push_back(std::make_pair(event, now));
    if (dmsEvents_.size() > MAX_EVENTS) {
        dmsEvents_.pop_front();
    }
    ZLOGI("Received dms event, eventType: %{public}d, srcNetworkId: %{public}s, srcBundleName: %{public}s, "
        "dstNetworkId: %{public}s, destBundleName: %{public}s", event.dSchedEventType_,
        DistributedData::Anonymous::Change(event.srcNetworkId_).c_str(), event.srcBundleName_.c_str(),
        DistributedData::Anonymous::Change(event.dstNetworkId_).c_str(), event.destBundleName_.c_str());
}

bool ObjectDmsHandler::IsContinue(const std::string &bundleName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto validityTime = std::chrono::steady_clock::now() - std::chrono::seconds(VALIDITY);
    DistributedHardware::DmDeviceInfo localDeviceInfo;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, localDeviceInfo);
    for (auto it = dmsEvents_.rbegin(); it != dmsEvents_.rend(); ++it) {
        if (it->second < validityTime) {
            continue;
        }
        if (it->first.dSchedEventType_ != DistributedSchedule::DMS_CONTINUE) {
            continue;
        }
        if (it->first.srcNetworkId_ == localDeviceInfo.networkId && it->first.srcBundleName_ == bundleName) {
            ZLOGI("Continue source, networkId: %{public}s, bundleName: %{public}s",
                DistributedData::Anonymous::Change(localDeviceInfo.networkId).c_str(), bundleName.c_str());
            return true;
        }
        if (it->first.dstNetworkId_ == localDeviceInfo.networkId && it->first.destBundleName_ == bundleName) {
            ZLOGI("Continue destination, networkId: %{public}s, bundleName: %{public}s",
                DistributedData::Anonymous::Change(localDeviceInfo.networkId).c_str(), bundleName.c_str());
            return true;
        }
    }
    ZLOGW("Not in continue, networkId: %{public}s, bundleName: %{public}s",
        DistributedData::Anonymous::Change(localDeviceInfo.networkId).c_str(), bundleName.c_str());
    return false;
}

std::string ObjectDmsHandler::GetDstBundleName(const std::string &srcBundleName, const std::string &dstNetworkId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto validityTime = std::chrono::steady_clock::now() - std::chrono::seconds(VALIDITY);
    DistributedHardware::DmDeviceInfo localDeviceInfo;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, localDeviceInfo);
    for (auto it = dmsEvents_.rbegin(); it != dmsEvents_.rend(); ++it) {
        if (it->second < validityTime) {
            continue;
        }
        if (it->first.dSchedEventType_ != DistributedSchedule::DMS_CONTINUE) {
            continue;
        }
        if (it->first.srcNetworkId_ == localDeviceInfo.networkId && it->first.srcBundleName_ == srcBundleName &&
            it->first.dstNetworkId_ == dstNetworkId) {
            ZLOGI("In continue, srcBundleName: %{public}s, dstBundleName: %{public}s",
                srcBundleName.c_str(), it->first.destBundleName_.c_str());
            return it->first.destBundleName_;
        }
    }
    ZLOGW("Not in continue, srcBundleName: %{public}s, srcNetworkId: %{public}s",
        srcBundleName.c_str(), DistributedData::Anonymous::Change(localDeviceInfo.networkId).c_str());
    return srcBundleName;
}

void ObjectDmsHandler::RegisterDmsEvent()
{
    ZLOGI("Start register dms event");
    if (dmsEventListener_ == nullptr) {
        dmsEventListener_ = sptr<DistributedSchedule::IDSchedEventListener>(new DmsEventListener);
    }
    if (dmsEventListener_ == nullptr) {
        ZLOGE("Register dms event listener failed, listener is nullptr");
        return;
    }
    auto status = DistributedSchedule::DmsHandler::GetInstance().RegisterDSchedEventListener(
        DistributedSchedule::DSchedEventType::DMS_ALL, dmsEventListener_);
    if (status != 0) {
        ZLOGE("Register dms event listener failed, status: %{public}d", status);
    }
}
}