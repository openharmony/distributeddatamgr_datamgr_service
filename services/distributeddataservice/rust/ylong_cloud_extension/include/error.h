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

#ifndef CLOUD_EXTENSION_ERROR_H
#define CLOUD_EXTENSION_ERROR_H


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define ERRNO_SUCCESS 0
#define ERRNO_NULLPTR 1
#define ERRNO_WRONG_TYPE 2
#define ERRNO_INVALID_INPUT_TYPE 3
#define ERRNO_ASSET_DOWNLOAD_FAILURE 4
#define ERRNO_ASSET_UPLOAD_FAILURE 5
#define ERRNO_UNSUPPORTED 6
#define ERRNO_NETWORK_ERROR 7
#define ERRNO_LOCKED_BY_OTHERS 8
#define ERRNO_UNLOCKED 9
#define ERRNO_RECORD_LIMIT_EXCEEDED 10
#define ERRNO_NO_SPACE_FOR_ASSET 11
#define ERRNO_IPC_RW_ERROR 12
#define ERRNO_IPC_CONN_ERROR 13
#define ERRNO_IPC_ERRORS 14
#define ERRNO_OTHER_IO_ERROR 15
#define ERRNO_OUT_OF_RANGE 16
#define ERRNO_NO_SUCH_TABLE_IN_DB 17
#define ERRNO_CLOUD_INVALID_STATUS 18
#define ERRNO_UNKNOWN 19
#define ERRNO_CLOUD_DISABLE 20


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // CLOUD_EXTENSION_ERROR_H
