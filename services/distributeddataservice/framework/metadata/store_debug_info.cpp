/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "metadata/store_debug_info.h"
#include "utils/constant.h"
namespace OHOS::DistributedData {
bool StoreDebugInfo::FileInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(inode)], inode);
    SetValue(node[GET_NAME(size)], size);
    SetValue(node[GET_NAME(dev)], dev);
    SetValue(node[GET_NAME(mode)], mode);
    SetValue(node[GET_NAME(uid)], uid);
    SetValue(node[GET_NAME(gid)], gid);
    return true;
}

bool StoreDebugInfo::FileInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(inode), inode);
    GetValue(node, GET_NAME(size), size);
    GetValue(node, GET_NAME(dev), dev);
    GetValue(node, GET_NAME(mode), mode);
    GetValue(node, GET_NAME(uid), uid);
    GetValue(node, GET_NAME(gid), gid);
    return true;
}

bool StoreDebugInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(fileInfos)], fileInfos);
    return true;
}

bool StoreDebugInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(fileInfos), fileInfos);
    return true;
}

std::string StoreDebugInfo::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, fields);
}
}