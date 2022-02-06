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
#include "utils/crypto.h"
#include <random>
#include "openssl/sha.h"
namespace OHOS {
namespace DistributedData {
std::string Crypto::Sha256(const std::string &text)
{
    unsigned char hash[SHA256_DIGEST_LENGTH] = "";
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, text.c_str(), text.size());
    SHA256_Final(hash, &ctx);
    std::string hashToHex;
    // here we translate sha256 hash to hexadecimal. each 8-bit char will be presented by two characters([0-9a-f])
    constexpr int CHAR_WIDTH = 8;
    constexpr int HEX_WIDTH = 4;
    constexpr unsigned char HEX_MASK = 0xf;
    constexpr int HEX_A = 10;
    hashToHex.reserve(SHA256_DIGEST_LENGTH * (CHAR_WIDTH / HEX_WIDTH));
    for (unsigned char i : hash) {
        unsigned char hex = i >> HEX_WIDTH;
        if (hex < HEX_A) {
            hashToHex.push_back('0' + hex);
        } else {
            hashToHex.push_back('a' + hex - HEX_A);
        }
        hex = i & HEX_MASK;
        if (hex < HEX_A) {
            hashToHex.push_back('0' + hex);
        } else {
            hashToHex.push_back('a' + hex - HEX_A);
        }
    }
    return hashToHex;
}

std::vector<uint8_t> Crypto::Random(int32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (int32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}
}
}