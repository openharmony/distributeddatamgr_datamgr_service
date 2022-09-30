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

#ifndef DISTRIBUTEDDATAFWK_IDATA_SHARE_SERVICE_H
#define DISTRIBUTEDDATAFWK_IDATA_SHARE_SERVICE_H

#include <iremote_broker.h>
#include <string>

#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "datashare_values_bucket.h"

namespace OHOS::DataShare {
class IDataShareService {
public:
    enum {
        DATA_SHARE_SERVICE_CMD_INSERT,
        DATA_SHARE_SERVICE_CMD_DELETE,
        DATA_SHARE_SERVICE_CMD_UPDATE,
        DATA_SHARE_SERVICE_CMD_QUERY,
        DATA_SHARE_SERVICE_CMD_MAX
    };

    enum { DATA_SHARE_ERROR = -1, DATA_SHARE_OK = 0 };
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DataShare.IDataShareService");

    virtual int32_t Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket) = 0;
    virtual int32_t Update(
        const std::string &uri, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket) = 0;
    virtual int32_t Delete(const std::string &uri, const DataSharePredicates &predicate) = 0;
    virtual std::shared_ptr<DataShareResultSet> Query(
        const std::string &uri, const DataSharePredicates &predicates, const std::vector<std::string> &columns) = 0;
};
} // namespace OHOS::DataShare
#endif
