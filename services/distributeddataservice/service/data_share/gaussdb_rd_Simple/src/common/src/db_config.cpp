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

#include <algorithm>
#include <memory>

#include "doc_errno.h"
#include "doc_limit.h"
#include "log_print.h"
#include "json_object.h"

namespace DocumentDB {

namespace {
bool CheckPageSizeConfig(std::shared_ptr<JsonObject> config, int32_t &pageSize, int &errCode)
{
    static const JsonFieldPath pageSizeField = {"pageSize"};
    if (!config->IsFieldExists(pageSizeField)) {
        return true;
    }

    ValueObject configValue;
    errCode = config->GetObjectByPath(pageSizeField, configValue); // TODO: check return code
    if (configValue.valueType != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of pageSize is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    static const std::vector<int32_t> pageSizeValid = {4, 8, 16, 32, 64};
    if (std::find(pageSizeValid.begin(), pageSizeValid.end(), configValue.intValue) == pageSizeValid.end()) {
        GLOGE("Check DB config failed, invalid pageSize value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    pageSize = static_cast<int32_t>(configValue.intValue);
    return true;
}

bool CheckRedoFlushConfig(std::shared_ptr<JsonObject> config, uint32_t &redoFlush, int &errCode)
{
    static const JsonFieldPath redoFlushField = {"redoFlushByTrx"};
    if (!config->IsFieldExists(redoFlushField)) {
        return true;
    }

    ValueObject configValue;
    config->GetObjectByPath(redoFlushField, configValue);
    if (configValue.valueType != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of redoFlushByTrx is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.intValue != 0 && configValue.intValue != 1) {
        GLOGE("Check DB config failed, invalid redoFlushByTrx value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    redoFlush = static_cast<uint32_t>(configValue.intValue);
    return true;
}

bool CheckRedoBufSizeConfig(std::shared_ptr<JsonObject> config, uint32_t &redoBufSize, int &errCode)
{
    static const JsonFieldPath redoBufSizeField = {"redoPubBufSize"};
    if (!config->IsFieldExists(redoBufSizeField)) {
        return true;
    }

    ValueObject configValue;
    config->GetObjectByPath(redoBufSizeField, configValue);
    if (configValue.valueType != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of redoPubBufSize is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.intValue < 256 && configValue.intValue > 16384) {
        GLOGE("Check DB config failed, invalid redoPubBufSize value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    redoBufSize = static_cast<uint32_t>(configValue.intValue);
    return true;
}

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

    maxConnNum = static_cast<int32_t>(configValue.intValue);
    return true;
}

bool CheckBufferPoolSizeConfig(std::shared_ptr<JsonObject> config, int32_t pageSize, uint32_t &redoBufSize,
    int &errCode)
{
    static const JsonFieldPath bufferPoolSizeField = {"bufferPoolSize"};
    if (!config->IsFieldExists(bufferPoolSizeField)) {
        return true;
    }

    ValueObject configValue;
    config->GetObjectByPath(bufferPoolSizeField, configValue);
    if (configValue.valueType != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of bufferPoolSize is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.intValue < 1024 && configValue.intValue > 4 * 1024 * 1024 || configValue.intValue < pageSize * 33) {
        GLOGE("Check DB config failed, invalid bufferPoolSize value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    redoBufSize = static_cast<uint32_t>(configValue.intValue);
    return true;
}

bool CheckCrcCheckEnableConfig(std::shared_ptr<JsonObject> config, uint32_t &crcCheckEnable, int &errCode)
{
    static const JsonFieldPath crcCheckEnableField = {"crcCheckEnable"};
    if (!config->IsFieldExists(crcCheckEnableField)) {
        return true;
    }

    ValueObject configValue;
    config->GetObjectByPath(crcCheckEnableField, configValue);
    if (configValue.valueType != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of crcCheckEnable is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.intValue != 0 && configValue.intValue != 1) {
        GLOGE("Check DB config failed, invalid crcCheckEnable value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    crcCheckEnable = static_cast<uint32_t>(configValue.intValue);
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
        GLOGE("Read DB config failed from str. %d", errCode);
        return {};
    }

    DBConfig conf;
    if (!CheckPageSizeConfig(dbConfig, conf.pageSize_, errCode)) {
        GLOGE("Check DB config 'pageSize' failed. %d", errCode);
        return {};
    }

    if (!CheckRedoFlushConfig(dbConfig, conf.redoFlushByTrx_, errCode)) {
        GLOGE("Check DB config 'redoFlushByTrx' failed. %d", errCode);
        return {};
    }

    if (!CheckRedoBufSizeConfig(dbConfig, conf.redoPubBufSize_, errCode)) {
        GLOGE("Check DB config 'redoPubBufSize' failed. %d", errCode);
        return {};
    }

    if (!CheckMaxConnNumConfig(dbConfig, conf.maxConnNum_, errCode)) {
        GLOGE("Check DB config 'maxConnNum' failed. %d", errCode);
        return {};
    }

    if (!CheckBufferPoolSizeConfig(dbConfig, conf.pageSize_, conf.bufferPoolSize_, errCode)) {
        GLOGE("Check DB config 'bufferPoolSize' failed. %d", errCode);
        return {};
    }

    if (!CheckCrcCheckEnableConfig(dbConfig, conf.crcCheckEnable_, errCode)) {
        GLOGE("Check DB config 'crcCheckEnable' failed. %d", errCode);
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

int32_t DBConfig::GetPageSize() const
{
    return pageSize_;
}

bool DBConfig::operator==(const DBConfig &targetConfig) const
{
    return configStr_ == targetConfig.configStr_ && pageSize_ == targetConfig.pageSize_ &&
        redoFlushByTrx_ == targetConfig.redoFlushByTrx_ && redoPubBufSize_ == targetConfig.redoPubBufSize_ &&
        maxConnNum_ == targetConfig.maxConnNum_ && bufferPoolSize_ == targetConfig.bufferPoolSize_ &&
        crcCheckEnable_ == targetConfig.crcCheckEnable_;
}

bool DBConfig::operator!=(const DBConfig &targetConfig) const
{
    return !(*this == targetConfig);
}
} // namespace DocumentDB