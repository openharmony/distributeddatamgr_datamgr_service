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
#include "projection_tree.h"

namespace DocumentDB {
constexpr int JSON_DEEP_MAX = 4;

int ProjectionTree::ParseTree(std::vector<std::vector<std::string>> &path)
{
    ProjectionNode *node = &node_;
    if (node == nullptr) {
        return E_OK;
    }
    for (auto singlePath : path) {
        node = &node_;
        for (size_t j = 0; j < singlePath.size(); j++) {
            if (node->sonNode[singlePath[j]] != nullptr) {
                node = node->sonNode[singlePath[j]];
                if (j < singlePath.size() - 1 && node->isDeepest) {
                    return -E_INVALID_ARGS;
                }
                if (j == singlePath.size() - 1 && !node->isDeepest) {
                    return -E_INVALID_ARGS;
                }
            } else {
                auto tempNode = new (std::nothrow) ProjectionNode;
                if (tempNode == nullptr) {
                    GLOGE("Memory allocation failed!");
                    return -E_FAILED_MEMORY_ALLOCATE;
                }
                tempNode->Deep = node->Deep + 1;
                if (tempNode->Deep > JSON_DEEP_MAX) {
                    delete tempNode;
                    return -E_INVALID_ARGS;
                }
                node->isDeepest = false;
                node->sonNode[singlePath[j]] = tempNode;
                node = node->sonNode[singlePath[j]];
            }
        }
    }
    return E_OK;
}

bool ProjectionTree::SearchTree(std::vector<std::string> &singlePath, size_t &index)
{
    ProjectionNode *node = &node_;
    for (size_t i = 0; i < singlePath.size(); i++) {
        if (node->isDeepest) {
            index = i;
        }
        if (node->sonNode[singlePath[i]] != nullptr) {
            node = node->sonNode[singlePath[i]];
        } else {
            return false;
        }
    }
    return true;
}

int ProjectionNode::DeleteProjectionNode()
{
    for (auto item : sonNode) {
        if (item.second != nullptr) {
            delete item.second;
            item.second = nullptr;
        }
    }
    return E_OK;
}
} // namespace DocumentDB