#include "projection_tree.h"

#include <iostream>

namespace DocumentDB {
const int JSON_DEEP_MAX = 4;

ProjectionTree::ProjectionTree() {}

ProjectionTree::~ProjectionTree() {}

int ProjectionTree::ParseTree(std::vector<std::vector<std::string>> &path)
{
    ProjectionNode *node = &node_;
    if (node == NULL) {
        return E_OK;
    }
    for (int i = 0; i < path.size(); i++) {
        node = &node_;
        for (int j = 0; j < path[i].size(); j++) {
            if (node->SonNode[path[i][j]] != nullptr) {
                node = node->SonNode[path[i][j]];
                if (j < path[i].size() - 1 && node->isDeepest) {
                    return -E_INVALID_ARGS;
                }
                if (j == path[i].size() - 1 && !node->isDeepest) {
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
                node->SonNode[path[i][j]] = tempNode;
                node = node->SonNode[path[i][j]];
            }
        }
    }
    return E_OK;
}

bool ProjectionTree::SearchTree(std::vector<std::string> &singlePath, int &index)
{
    ProjectionNode *node = &node_;
    for (int i = 0; i < singlePath.size(); i++) {
        if (node->isDeepest) {
            index = i;
        }
        if (node->SonNode[singlePath[i]] != nullptr) {
            node = node->SonNode[singlePath[i]];
        } else {
            return false;
        }
    }
    return true;
}

int ProjectionNode::DeleteProjectionNode()
{
    for (auto item : SonNode) {
        if (item.second != nullptr) {
            delete item.second;
            item.second = nullptr;
        }
    }
    return E_OK;
}
} // namespace DocumentDB