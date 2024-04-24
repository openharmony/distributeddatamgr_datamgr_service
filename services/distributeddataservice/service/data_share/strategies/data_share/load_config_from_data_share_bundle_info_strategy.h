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

#ifndef DATASHARESERVICE_LOAD_CONFIG_FROM_DATA_SHARE_BUNDLE_INFO_STRAGETY_H
#define DATASHARESERVICE_LOAD_CONFIG_FROM_DATA_SHARE_BUNDLE_INFO_STRAGETY_H

#include "data_share_profile_config.h"
#include "strategy.h"
namespace OHOS::DataShare {
class LoadConfigFromDataShareBundleInfoStrategy final: public Strategy {
public:
    LoadConfigFromDataShareBundleInfoStrategy() = default;
    bool operator()(std::shared_ptr<Context> context) override;

private:
    bool LoadConfigFromProfile(const ProfileInfo &profileInfo, std::shared_ptr<Context> context);
    bool LoadConfigFromUri(std::shared_ptr<Context> context);
    static constexpr const char *DATA_SHARE_EXTENSION_META = "ohos.extension.dataShare";
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_LOAD_CONFIG_FROM_DATA_SHARE_BUNDLE_INFO_STRAGETY_H
