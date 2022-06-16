/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATA_SERVICE_DUMPE_HELPER_H
#define DISTRIBUTEDDATA_SERVICE_DUMPE_HELPER_H

#include <string>
#include <vector>
#include <map>
#include <list>
#include <functional>
#include "store_errno.h"
#include "singleton.h"

namespace OHOS {
namespace DistributedKv {
class DumpHelper : public Singleton<DumpHelper> {
public:
    using DumpNoParamFunc = std::function<void(int)>;
    using DumpWithParamFunc = std::function<void(int, const std::string &)>;
    DumpHelper() = default;
    virtual ~DumpHelper() = default;
    void AddDumpOperation(const DumpNoParamFunc &dumpAll, const DumpNoParamFunc &dumpUserInfo,
        const DumpWithParamFunc &dumpAppInfo, const DumpWithParamFunc &dumpStoreInfo);
    void AddErrorInfo(const std::string &error);
    void ShowError(int fd);
    bool Dump(int fd, const std::vector<std::string> &args);
private:
    void ShowHelp(int fd);
    void ShowIllealInfomation(int fd);
    mutable std::mutex hidumperMutex_;
    std::list<std::string> g_errorInfo;
    DumpNoParamFunc dumpAll_;
    DumpNoParamFunc dumpUserInfo_;
    DumpWithParamFunc dumpAppInfo_;
    DumpWithParamFunc dumpStoreInfo_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // DISTRIBUTEDDATA_SERVICE_DUMPE_HELPER_H