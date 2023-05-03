#ifndef PROJECTION_TREE_H
#define PROJECTION_TREE_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include "doc_errno.h"
#include "json_common.h"
#include "log_print.h"

namespace DocumentDB {
struct ProjectionNode {
    std::unordered_map<std::string, ProjectionNode*> SonNode;
    bool isDeepest;
    int Deep;
    int ViewType;
    ProjectionNode() {
        Deep = 0;
        isDeepest = true;
    }
    int DeleteProjectionNode();
    ~ProjectionNode () {
        DeleteProjectionNode();
    }
};
class ProjectionTree {
public:
    ProjectionTree();
    ~ProjectionTree();

    int ParseTree(std::vector<std::vector<std::string>> &path);
    bool SearchTree(std::vector<std::string> &singlePath, int &index);
private:
    ProjectionNode node_;
};
} // DocumentDB
#endif // PROJECTION_TREE_H