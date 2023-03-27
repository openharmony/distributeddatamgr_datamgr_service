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

#include "db_config.h"
#include "doc_errno.h"
#include "doc_limit.h"
#include "log_print.h"
#include "json_object.h"

namespace DocumentDB {

namespace {
bool CheckMaxConnNumConfig(std::shared_ptr<JsonObject> config, int32_t &maxConnNum, int &errCode)
{
    static const JsonFieldPath maxConnNumField = {"maxConnNum"};
    if (!config->IsFieldExists(maxConnNumField)) {
        return true;
    }

    ValueObject configValue;
    config->GetObjectByPath(maxConnNumField, configValue);
    if (configValue.valueType != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of maxConnNum is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.intValue < 16 || configValue.intValue > 1024) {
        GLOGE("Check DB config failed, invalid maxConnNum value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    maxConnNum = configValue.intValue;
    return true;
}
}

DBConfig DBConfig::ReadConfig(const std::string &confStr, int &errCode)
{
    if (confStr.empty()) {
        return {};
    }

    if (confStr.length() > MAX_DB_CONFIG_LEN) {
        GLOGE("Config json string is too long.");
        errCode = -E_OVER_LIMIT;
        return {};
    }
    std::shared_ptr<JsonObject> dbConfig;
    errCode = JsonObject::Parse(confStr, dbConfig);
    if (errCode != E_OK) {
        GLOGE("Read DB config failed from. %d", errCode);
        return {};
    }

    DBConfig conf;
    if (!CheckMaxConnNumConfig(dbConfig, conf.maxConnNum_, errCode)) {
        GLOGE("Check DB config 'maxConnNum' failed. %d", errCode);
        return {};
    }

    conf.configStr_ = confStr;
    errCode = E_OK;
    return conf;
}

std::string DBConfig::ToString() const
{
    return configStr_;
}

int32_t DBConfig::GetMaxConnNum() const
{
    return maxConnNum_;
}

int32_t DBConfig::GetPageSize() const
{
    return pageSize_;
}

bool DBConfig::operator==(const DBConfig &targetConfig) const
{
    return configStr_ == targetConfig.configStr_ && pageSize_ == targetConfig.pageSize_ &&
        redoFlushByTrx_ == targetConfig.redoFlushByTrx_ && redoPubBufSize_ == targetConfig.redoPubBufSize_ &&
        maxConnNum_ == targetConfig.maxConnNum_ && spaceMaxSize_ == targetConfig.spaceMaxSize_ &&
        bufferPoolSize_ == targetConfig.bufferPoolSize_ && crcCheckEnable_ == targetConfig.crcCheckEnable_;
}

bool DBConfig::operator!=(const DBConfig &targetConfig) const
{
    return !(*this == targetConfig);
}
} // namespace DocumentDB