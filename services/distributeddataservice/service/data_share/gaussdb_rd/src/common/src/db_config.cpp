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
#include <cstring>
#include <functional>
#include <memory>

#include "doc_errno.h"
#include "doc_limit.h"
#include "json_object.h"
#include "log_print.h"

namespace DocumentDB {
namespace {
const int MIN_REDO_BUFFER_SIZE = 256;
const int MAX_REDO_BUFFER_SIZE = 16384;
const int MIN_CONNECTION_NUM = 16;
const int MAX_CONNECTION_NUM = 1024;
const int MIN_BUFFER_POOL_SIZE = 1024;
const int MAX_BUFFER_POOL_SIZE = 4 * 1024 * 1024;

constexpr const char *DB_CONFIG_PAGESIZE = "pagesize";
constexpr const char *DB_CONFIG_REDO_FLUSH_BY_TRX = "redoflushbytrx";
constexpr const char *DB_CONFIG_REDO_PUB_BUFF_SIZE = "redopubbufsize";
constexpr const char *DB_CONFIG_MAX_CONN_NUM = "maxconnnum";
constexpr const char *DB_CONFIG_BUFFER_POOL_SIZE = "bufferpoolsize";
constexpr const char *DB_CONFIG_CRC_CHECK_ENABLE = "crccheckenable";

const int DB_CONFIG_SIZE = 6; // db config size
const char *DB_CONFIG[DB_CONFIG_SIZE] = { DB_CONFIG_PAGESIZE, DB_CONFIG_REDO_FLUSH_BY_TRX,
    DB_CONFIG_REDO_PUB_BUFF_SIZE, DB_CONFIG_MAX_CONN_NUM, DB_CONFIG_BUFFER_POOL_SIZE, DB_CONFIG_CRC_CHECK_ENABLE };

template<typename T>
bool CheckAndGetDBConfig(const JsonObject &config, const std::string &name, const std::function<bool(T)> &checkValid,
    T &val)
{
    const JsonFieldPath configField = { name };
    if (!config.IsFieldExists(configField)) {
        return true;
    }

    int errCode = E_OK;
    ValueObject configValue = config.GetObjectByPath(configField, errCode);
    if (configValue.GetValueType() != ValueObject::ValueType::VALUE_NUMBER) {
        GLOGE("Check DB config failed, not found or type of %s is not NUMBER.", name.c_str());
        return false;
    }

    if (checkValid && !checkValid(static_cast<T>(configValue.GetIntValue()))) {
        GLOGE("Check DB config failed, invalid %s value.", name.c_str());
        return false;
    }

    val = static_cast<T>(configValue.GetIntValue());
    return true;
}

bool CheckPageSizeConfig(const JsonObject &config, int32_t &pageSize)
{
    std::function<bool(int32_t)> checkFunction = [](int32_t val) {
        static const std::vector<int32_t> pageSizeValid = { 4, 8, 16, 32, 64 };
        return std::find(pageSizeValid.begin(), pageSizeValid.end(), val) != pageSizeValid.end();
    };
    return CheckAndGetDBConfig(config, DB_CONFIG_PAGESIZE, checkFunction, pageSize);
}

bool CheckRedoFlushConfig(const JsonObject &config, uint32_t &redoFlush)
{
    std::function<bool(uint32_t)> checkFunction = [](uint32_t val) {
        return val == 0 || val == 1;
    };
    return CheckAndGetDBConfig(config, DB_CONFIG_REDO_FLUSH_BY_TRX, checkFunction, redoFlush);
}

bool CheckRedoBufSizeConfig(const JsonObject &config, uint32_t &redoBufSize)
{
    std::function<bool(uint32_t)> checkFunction = [](uint32_t val) {
        return val >= MIN_REDO_BUFFER_SIZE && val <= MAX_REDO_BUFFER_SIZE;
    };
    return CheckAndGetDBConfig(config, DB_CONFIG_REDO_PUB_BUFF_SIZE, checkFunction, redoBufSize);
}

bool CheckMaxConnNumConfig(const JsonObject &config, int32_t &maxConnNum)
{
    std::function<bool(int32_t)> checkFunction = [](int32_t val) {
        return val >= MIN_CONNECTION_NUM && val <= MAX_CONNECTION_NUM;
    };
    return CheckAndGetDBConfig(config, DB_CONFIG_MAX_CONN_NUM, checkFunction, maxConnNum);
}

bool CheckBufferPoolSizeConfig(const JsonObject &config, int32_t pageSize, uint32_t &redoBufSize)
{
    std::function<bool(uint32_t)> checkFunction = [&pageSize](uint32_t val) {
        return val >= MIN_BUFFER_POOL_SIZE && val <= MAX_BUFFER_POOL_SIZE &&
            val >= static_cast<uint32_t>(pageSize * 64); // 64: pool size should be 64 times larger then pageSize
    };
    return CheckAndGetDBConfig(config, DB_CONFIG_BUFFER_POOL_SIZE, checkFunction, redoBufSize);
}

bool CheckCrcCheckEnableConfig(const JsonObject &config, uint32_t &crcCheckEnable)
{
    std::function<bool(uint32_t)> checkFunction = [](uint32_t val) {
        return val == 0 || val == 1;
    };
    return CheckAndGetDBConfig(config, DB_CONFIG_BUFFER_POOL_SIZE, checkFunction, crcCheckEnable);
}

int CheckConfigValid(const JsonObject &config)
{
    JsonObject child = config.GetChild();
    while (!child.IsNull()) {
        std::string fieldName = child.GetItemField();
        bool isSupport = false;
        for (int i = 0; i < DB_CONFIG_SIZE; i++) {
            if (strcmp(DB_CONFIG[i], fieldName.c_str()) == 0) {
                isSupport = true;
                break;
            }
        }

        if (!isSupport) {
            GLOGE("Invalid db config.");
            return -E_INVALID_CONFIG_VALUE;
        }

        child = child.GetNext();
    }
    return E_OK;
}
} // namespace

DBConfig DBConfig::GetDBConfigFromJsonStr(const std::string &confStr, int &errCode)
{
    JsonObject dbConfig = JsonObject::Parse(confStr, errCode);
    if (errCode != E_OK) {
        GLOGE("Read DB config failed from str. %d", errCode);
        return {};
    }

    errCode = CheckConfigValid(dbConfig);
    if (errCode != E_OK) {
        GLOGE("Check DB config, not support config item. %d", errCode);
        return {};
    }

    DBConfig conf;
    if (!CheckPageSizeConfig(dbConfig, conf.pageSize_)) {
        GLOGE("Check DB config 'pageSize' failed.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    if (!CheckRedoFlushConfig(dbConfig, conf.redoFlushByTrx_)) {
        GLOGE("Check DB config 'redoFlushByTrx' failed.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    if (!CheckRedoBufSizeConfig(dbConfig, conf.redoPubBufSize_)) {
        GLOGE("Check DB config 'redoPubBufSize' failed.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    if (!CheckMaxConnNumConfig(dbConfig, conf.maxConnNum_)) {
        GLOGE("Check DB config 'maxConnNum' failed.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    if (!CheckBufferPoolSizeConfig(dbConfig, conf.pageSize_, conf.bufferPoolSize_)) {
        GLOGE("Check DB config 'bufferPoolSize' failed.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    if (!CheckCrcCheckEnableConfig(dbConfig, conf.crcCheckEnable_)) {
        GLOGE("Check DB config 'crcCheckEnable' failed.");
        errCode = -E_INVALID_CONFIG_VALUE;
        return {};
    }

    conf.configStr_ = confStr;
    errCode = E_OK;
    return conf;
}

DBConfig DBConfig::ReadConfig(const std::string &confStr, int &errCode)
{
    if (confStr.empty()) {
        return {};
    }

    if (confStr.length() + 1 > MAX_DB_CONFIG_LEN) {
        GLOGE("Config json string is too long.");
        errCode = -E_OVER_LIMIT;
        return {};
    }

    std::string lowerCaseConfStr = confStr;
    std::transform(lowerCaseConfStr.begin(), lowerCaseConfStr.end(), lowerCaseConfStr.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return GetDBConfigFromJsonStr(lowerCaseConfStr, errCode);
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
    return pageSize_ == targetConfig.pageSize_ && redoFlushByTrx_ == targetConfig.redoFlushByTrx_ &&
        redoPubBufSize_ == targetConfig.redoPubBufSize_ && maxConnNum_ == targetConfig.maxConnNum_ &&
        bufferPoolSize_ == targetConfig.bufferPoolSize_ && crcCheckEnable_ == targetConfig.crcCheckEnable_;
}

bool DBConfig::operator!=(const DBConfig &targetConfig) const
{
    return !(*this == targetConfig);
}

bool DBConfig::CheckPersistenceEqual(const DBConfig &targetConfig) const
{
    return pageSize_ == targetConfig.pageSize_ && crcCheckEnable_ == targetConfig.crcCheckEnable_;
}
} // namespace DocumentDB