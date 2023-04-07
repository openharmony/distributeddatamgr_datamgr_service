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
bool CheckPageSizeConfig(const JsonObject &config, int32_t &pageSize, int &errCode)
{
    static const JsonFieldPath pageSizeField = {"pageSize"};
    if (!config.IsFieldExists(pageSizeField)) {
        return true;
    }

    ValueObject configValue = config.GetObjectByPath(pageSizeField, errCode); // TODO: check return code
    if (configValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of pageSize is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    static const std::vector<int32_t> pageSizeValid = {4, 8, 16, 32, 64};
    if (std::find(pageSizeValid.begin(), pageSizeValid.end(), configValue.GetIntValue()) == pageSizeValid.end()) {
        GLOGE("Check DB config failed, invalid pageSize value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    pageSize = static_cast<int32_t>(configValue.GetIntValue());
    return true;
}

bool CheckRedoFlushConfig(const JsonObject &config, uint32_t &redoFlush, int &errCode)
{
    static const JsonFieldPath redoFlushField = {"redoFlushByTrx"};
    if (!config.IsFieldExists(redoFlushField)) {
        return true;
    }

    ValueObject configValue = config.GetObjectByPath(redoFlushField, errCode); // TODO:
    if (configValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of redoFlushByTrx is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.GetIntValue() != 0 && configValue.GetIntValue() != 1) {
        GLOGE("Check DB config failed, invalid redoFlushByTrx value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    redoFlush = static_cast<uint32_t>(configValue.GetIntValue());
    return true;
}

bool CheckRedoBufSizeConfig(const JsonObject &config, uint32_t &redoBufSize, int &errCode)
{
    static const JsonFieldPath redoBufSizeField = {"redoPubBufSize"};
    if (!config.IsFieldExists(redoBufSizeField)) {
        return true;
    }

    ValueObject configValue = config.GetObjectByPath(redoBufSizeField, errCode); // TODO:
    if (configValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of redoPubBufSize is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.GetIntValue() < 256 && configValue.GetIntValue() > 16384) {
        GLOGE("Check DB config failed, invalid redoPubBufSize value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    redoBufSize = static_cast<uint32_t>(configValue.GetIntValue());
    return true;
}

bool CheckMaxConnNumConfig(const JsonObject &config, int32_t &maxConnNum, int &errCode)
{
    static const JsonFieldPath maxConnNumField = {"maxConnNum"};
    if (!config.IsFieldExists(maxConnNumField)) {
        return true;
    }

    ValueObject configValue = config.GetObjectByPath(maxConnNumField, errCode); // TODO:
    if (configValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of maxConnNum is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.GetIntValue() < 16 || configValue.GetIntValue() > 1024) {
        GLOGE("Check DB config failed, invalid maxConnNum value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    maxConnNum = static_cast<int32_t>(configValue.GetIntValue());
    return true;
}

bool CheckBufferPoolSizeConfig(const JsonObject &config, int32_t pageSize, uint32_t &redoBufSize,
    int &errCode)
{
    static const JsonFieldPath bufferPoolSizeField = {"bufferPoolSize"};
    if (!config.IsFieldExists(bufferPoolSizeField)) {
        return true;
    }

    ValueObject configValue = config.GetObjectByPath(bufferPoolSizeField, errCode); // TODO:
    if (configValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of bufferPoolSize is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.GetIntValue() < 1024 && configValue.GetIntValue() > 4 * 1024 * 1024 ||
        configValue.GetIntValue() < pageSize * 33) {
        GLOGE("Check DB config failed, invalid bufferPoolSize value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    redoBufSize = static_cast<uint32_t>(configValue.GetIntValue());
    return true;
}

bool CheckCrcCheckEnableConfig(const JsonObject &config, uint32_t &crcCheckEnable, int &errCode)
{
    static const JsonFieldPath crcCheckEnableField = {"crcCheckEnable"};
    if (!config.IsFieldExists(crcCheckEnableField)) {
        return true;
    }

    ValueObject configValue = config.GetObjectByPath(crcCheckEnableField, errCode); // TODO:
    if (configValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, the field type of crcCheckEnable is not NUMBER. %d", errCode);
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    if (configValue.GetIntValue() != 0 && configValue.GetIntValue() != 1) {
        GLOGE("Check DB config failed, invalid crcCheckEnable value.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return false;
    }

    crcCheckEnable = static_cast<uint32_t>(configValue.GetIntValue());
    return true;
}

bool CheckConfigSupport(const JsonObject &config, int &errCode)
{
    static const std::vector<std::string> DB_CONFIG = {
        "pageSize",
        "redoFlushByTrx",
        "redoPubBufSize",
        "maxConnNum",
        "bufferPoolSize",
        "crcCheckEnable"
    };

    JsonObject child = config.GetChild();
    while (!child.IsNull()) {
        std::string fieldName = child.GetItemFiled();
        if (std::find(DB_CONFIG.begin(), DB_CONFIG.end(), fieldName) == DB_CONFIG.end()) {
            GLOGE("Invalid db config.");
            errCode = -E_INVALID_CONFIG_VALUE;
            return false;
        }
        child = child.GetNext();
    }
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

    JsonObject dbConfig = JsonObject::Parse(confStr, errCode);
    if (errCode != E_OK) {
        GLOGE("Read DB config failed from str. %d", errCode);
        return {};
    }

    if (!CheckConfigSupport(dbConfig, errCode)) {
        GLOGE("Check DB config, not support config item. %d", errCode);
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