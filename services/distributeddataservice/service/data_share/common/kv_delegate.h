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

#ifndef DATASHARESERVICE_KV_DELEGATE_H
#define DATASHARESERVICE_KV_DELEGATE_H

#include <mutex>
#include <string>

#include "db_delegate.h"

namespace OHOS::DataShare {
class KvDelegate final : public KvDBDelegate {
public:
    KvDelegate(const std::string &path);
    virtual ~KvDelegate();
    int32_t Upsert(const std::string &collectionName, const KvData &value) override;
    int32_t DeleteById(const std::string &collectionName, const Id &id) override;
    int32_t Get(const std::string &collectionName, const Id &id, std::string &value) override;

    int32_t Get(const std::string &collectionName, const std::string &filter, const std::string &projection,
        std::string &result) override;
    int32_t GetBatch(const std::string &collectionName, const std::string &filter, const std::string &projection,
        std::vector<std::string> &result) override;

private:
    bool Init();
    bool GetVersion(const std::string &collectionName, const std::string &filter, int &version);
    int64_t Upsert(const std::string &collectionName, const std::string &filter, const std::string &value);
    int64_t Delete(const std::string &collectionName, const std::string &filter);
    void Flush();
    std::string path_;
    bool isInitDone_ = false;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_KV_DELEGATE_H
