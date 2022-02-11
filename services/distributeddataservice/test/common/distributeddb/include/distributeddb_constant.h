/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDB_CONSTANT_DEFINE_H
#define DISTRIBUTEDDB_CONSTANT_DEFINE_H

#include <string>
#include "store_types.h"

namespace DistributedDBDataGenerator {
const int TEST_ID_1 = 1;
const int TEST_ID_2 = 2;
const int TEST_ID_3 = 3;
const int TEST_ID_4 = 4;
const int TEST_ID_5 = 5;
const int TEST_ID_6 = 6;
const int TEST_ID_7 = 7;
const int TEST_ID_8 = 8;
const int TEST_ID_9 = 9;
const int TEST_ID_10 = 10;
const int TEST_ID_11 = 11;
const int TEST_ID_12 = 12;
const int TEST_ID_13 = 13;
const int TEST_ID_14 = 14;
const int TEST_ID_15 = 15;
const int TEST_ID_16 = 16;
const int TEST_ID_17 = 17;
const int TEST_ID_18 = 18;
const int TEST_ID_19 = 19;
const int TEST_ID_20 = 20;
const int TEST_ID_21 = 21;
const int TEST_ID_22 = 22;
const int TEST_ID_23 = 23;
const int TEST_ID_24 = 24;
const int TEST_ID_25 = 25;
const int TEST_ID_26 = 26;
const int TEST_ID_27 = 27;
const int TEST_ID_28 = 28;
const int TEST_ID_29 = 29;
const int TEST_ID_30 = 30;
const int TEST_ID_31 = 31;
const int TEST_ID_32 = 32;
const int TEST_ID_33 = 33;
const int TEST_ID_34 = 34;
const int TEST_ID_35 = 35;
const int TEST_ID_36 = 36;
const int TEST_ID_37 = 37;
const int TEST_ID_38 = 38;
const int TEST_ID_39 = 39;
const int TEST_ID_40 = 40;
const int TEST_ID_41 = 41;
const int TEST_ID_42 = 42;
const int TEST_ID_43 = 43;
const int TEST_ID_44 = 44;
const int TEST_ID_45 = 45;
const int TEST_ID_46 = 46;
const int TEST_ID_47 = 47;
const int TEST_ID_48 = 48;
const int TEST_ID_49 = 49;
const int TEST_ID_50 = 50;
const int TEST_ID_51 = 51;
const int TEST_ID_52 = 52;

class DistributedDBConstant {
public:
    static const std::string NB_DIRECTOR; // default work dir.
    static const std::string NB_DATABASE_NAME;
    static const std::string NORMAL_COMMON_SCHEMA;
    static const std::string COMPATIBLE_FOR_NORMAL_COMMON_SCHEMA;
    static const std::string UNCOMPATIBLE_FOR_NORMAL_COMMON_SCHEMA;
    // default kvStoreDelegateManager's config.
    static const DistributedDB::KvStoreConfig CONFIG;
    static const std::string DB_FILE_DOCUMENT;
    static constexpr int UNLOCK_ACCESS = 0;
    static constexpr int LOCKED_ACCESS = 1;
    static constexpr uint32_t THIRTY_MINUTES = 1800;
    static constexpr uint32_t WAIT_UNTIL_CALLBACK_COME = 0;

// ************************  SYNC PACKET ************************************
    static constexpr int ZERO_PACKET = 0;
    static constexpr int ONE_PACKET = 1;
    static constexpr int TWO_PACKETS = 2;
    static constexpr int THREE_PACKETS = 3;
    static constexpr int FOUR_PACKETS = 4;
    static constexpr int EIGHT_PACKETS = 8;
    static constexpr int FIRST_PACKET = 1;
    static constexpr int THIRD_PACKET = 3;
    static constexpr int FIFTH_PACKET = 5;
    static constexpr int EIGHTH_PACKET = 8;
    static constexpr uint32_t THROW_PACKET_CNT_LEN = 168; // the length of data packet is greater than 168
    // the length of registering subscriber packet is greater than 111
    static constexpr uint32_t REGISTER_SUBSCRIBER_PACKET_LEN = 111;

// ************************  MILLISECOND ************************************
    static const unsigned int THREE_S_TO_MS = 3000;
    static const unsigned int FIVE_S_TO_MS = 5000;
    static const unsigned int EIGHT_S_TO_MS = 8000;
    static const unsigned int NINE_S_TO_MS = 9000;
    static const unsigned int TEN_S_TO_MS = 10000;
    static const unsigned int SIXTY_S_TO_MS = 60000;

// ************************    compression ratio   **************************
    static const unsigned int COMPRESSION_RATIO_ZERO = 0;
    static const unsigned int COMPRESSION_RATIO_TEN = 10;
    static const unsigned int COMPRESSION_RATIO_TWENTY = 20;
    static const unsigned int COMPRESSION_RATIO_THIRTY = 30;
    static const unsigned int COMPRESSION_RATIO_FORTY = 40;
    static const unsigned int COMPRESSION_RATIO_FIFTY = 50;
    static const unsigned int COMPRESSION_RATIO_SIXTY = 60;
    static const unsigned int COMPRESSION_RATIO_SEVENTY = 70;
    static const unsigned int COMPRESSION_RATIO_EIGHTY = 80;
    static const unsigned int COMPRESSION_RATIO_NINETY = 90;
    static const unsigned int COMPRESSION_RATIO_ONE_HUNDRED = 100;
    static const unsigned int COMPRESSION_RATIO_ONE_HUNDRED_AND_ONE = 101;

// ************************    record number   **************************
    static const unsigned int THIRTY_THOUSAND_RECORDS = 30000;
};
} // namespace DistributedDBDataGenerator

#endif // DISTRIBUTEDDB_CONSTANT_DEFINE_H
