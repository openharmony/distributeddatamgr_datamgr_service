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

#include "distributeddb_tools_test.h"
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <openssl/rand.h>
#include <random>
#include <set>
#include <sys/types.h>

#include "db_common.h"
#include "db_constant.h"
#include "generic_single_ver_kv_entry.h"
#include "platform_specific.h"
#include "single_ver_data_packet.h"
#include "value_hash_calc.h"

using namespace DistributedDB;
namespace DistributedDBTest {
    int DistributedDBToolsTest::GetCurrentDir(std::string &dir)
    {
        static const int maxFileLength = 1024;
        dir = "";
        char buffer[maxFileLength] = {0};
        int length = readlink("/proc/self/exe", buffer, maxFileLength);
        if (length < 0 || length >= maxFileLength) {
            LOGE("read directory err length:%d", length);
            return -E_LENGTH_ERROR;
        }
        LOGD("DIR = %s", buffer);
        dir = buffer;
        if (std::string::npos == dir.rfind("/") && std::string::npos == dir.rfind("\\")) {
            LOGE("current patch format err");
            return -E_INVALID_PATH;
        }

        if (dir.rfind("/") != std::string::npos) {
            dir.erase(dir.rfind("/") + 1);
        }
        return E_OK;
    }
    
    void DistributedDBToolsTest::TestDirInit(std::string& dir)
    {
        if (GetCurrentDir(dir) != E_OK) {
            dir = "/";
        }

        dir.append("testDbDir");
        DIR *dirTmp = opendir(dir.c_str());
        if (dirTmp == nullptr) {
            if (OS::MakeDBDirectory(dir) != 0) {
                LOGI("MakeDirectory err!");
                dir = "/";
                return;
            }
        } else {
            closedir(dirTmp);
        }
    }

    int DistributedDBToolsTest::RemoveTestDbFiles(const std::string& dir)
    {
        bool isExisted = OS::CheckPathExistence(dir);
        if (!isExisted) {
            return E_OK;
        }

        int nFile = 0;
        std::string dirName;
        struct dirent *direntPtr = nullptr;
        DIR *dirPtr = opendir(dir.c_str());
        if (dirPtr == nullptr) {
            LOGE("opendir error!");
            return -E_INVALID_PATH;
        }
        while (true) {
            direntPtr = readdir(dirPtr);
            // condition to exit the loop
            if (direntPtr == nullptr) {
                break;
            }
            // only remove all *.db files
            std::string str(direntPtr->d_name);
            if (str == "." || str == "..") {
                continue;
            }
            dirName.clear();
            dirName.append(dir).append("/").append(str);
            if (direntPtr->d_type == DT_DIR) {
                RemoveTestDbFiles(dirName);
                rmdir(dirName.c_str());
            } else if (remove(dirName.c_str()) != 0) {
                LOGI("remove file: %s failed!", dirName.c_str());
                continue;
            }
            nFile++;
        }
        closedir(dirPtr);
        LOGI("Total %d test db files are removed!", nFile);
        return 0;
    }
}