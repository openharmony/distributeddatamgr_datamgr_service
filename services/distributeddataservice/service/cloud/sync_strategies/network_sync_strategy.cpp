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

#define LOG_TAG "NetworkSyncStrategy"

#include "network_sync_strategy.h"
#include "metadata/meta_data_manager.h"
#include "device_manager_adapter.h"
#include "error/general_error.h"
#include "utils/constant.h"
#include "log_print.h"
namespace OHOS::CloudData {
using namespace OHOS::DistributedData;
NetworkSyncStrategy::NetworkSyncStrategy()
{
    MetaDataManager::GetInstance().Subscribe(
        StrategyInfo::PREFIX, [this](const std::string &key, const std::string &value, int32_t flag) -> auto {
            StrategyInfo info;
            StrategyInfo::Unmarshall(value, info);
            ZLOGI("flag:%{public}d, value:%{public}s", flag, value.c_str());
            if (info.user != user_) {
                return true;
            }
            if (flag == MetaDataManager::INSERT || flag == MetaDataManager::UPDATE) {
                strategies_.InsertOrAssign(info.bundleName, std::move(info));
            } else if (flag == MetaDataManager::DELETE) {
                strategies_.Erase(info.bundleName);
            } else {
                ZLOGE("ignored operation");
            }
            return true;
        }, true);
}
NetworkSyncStrategy::~NetworkSyncStrategy()
{
    MetaDataManager::GetInstance().Unsubscribe(StrategyInfo::PREFIX);
}
int32_t NetworkSyncStrategy::CheckSyncAction(const StoreInfo &storeInfo)
{
    if (!DeviceManagerAdapter::GetInstance().IsNetworkAvailable()) {
        return E_NETWORK_ERROR;
    }
    if (storeInfo.user != user_) {
        strategies_.Clear();
        user_ = storeInfo.user;
    }
    StrategyInfo info = GetStrategy(storeInfo.user, storeInfo.bundleName);
    if (info.user == StrategyInfo::INVALID_USER) {
        info = GetStrategy(storeInfo.user, GLOBAL_BUNDLE);
    }
    if (!Check(info.strategy)) {
        return E_BLOCKED_BY_NETWORK_STRATEGY;
    }
    return next_ ? next_->CheckSyncAction(storeInfo) : E_OK;
}

bool NetworkSyncStrategy::StrategyInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(user)], user);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(strategy)], strategy);
    return true;
}

bool NetworkSyncStrategy::StrategyInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(user), user);
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(strategy), strategy);
    return false;
}

std::string NetworkSyncStrategy::StrategyInfo::GetKey()
{
    return Constant::Join(StrategyInfo::PREFIX, Constant::KEY_SEPARATOR, { std::to_string(user), bundleName });
}

std::string NetworkSyncStrategy::GetKey(int32_t user)
{
    return Constant::Join(StrategyInfo::PREFIX, Constant::KEY_SEPARATOR, { std::to_string(user) });
}

std::string NetworkSyncStrategy::GetKey(int32_t user, const std::string &bundleName)
{
    return Constant::Join(StrategyInfo::PREFIX, Constant::KEY_SEPARATOR, { std::to_string(user), bundleName });
}

bool NetworkSyncStrategy::Check(uint32_t strategy)
{
    auto networkType = DeviceManagerAdapter::GetInstance().GetNetworkType();
    if (networkType == DeviceManagerAdapter::NONE) {
        networkType = DeviceManagerAdapter::GetInstance().GetNetworkType(true);
    }
    switch (networkType) {
        case DeviceManagerAdapter::WIFI:
        case DeviceManagerAdapter::ETHERNET:
            return (strategy & WIFI) == WIFI;
        case DeviceManagerAdapter::CELLULAR:
            return (strategy & CELLULAR) == CELLULAR;
        default:
            ZLOGD("verification failed! strategy:%{public}d, networkType:%{public}d", strategy, networkType);
            return false;
    }
}

NetworkSyncStrategy::StrategyInfo NetworkSyncStrategy::GetStrategy(int32_t user, const std::string &bundleName)
{
    auto [res, info] = strategies_.Find(bundleName);
    if (res) {
        return info;
    }
    MetaDataManager::GetInstance().LoadMeta(GetKey(user, bundleName), info, true);
    strategies_.Insert(info.bundleName, info);
    return info;
}
}
