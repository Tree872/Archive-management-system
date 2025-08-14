#pragma once
#include <stdbool.h>
#define TREE_BRANCH_MID "„¥"
#define TREE_VERTICAL "„ "
#define TREE_BRANCH_END "„¤"
#define TREE_HORIZONTAL "„Ÿ"
// Node representing a file or directory
typedef struct PathNode {
  char relativePath[256];               // 
  bool isFile;         // 1 if file, 0 if directory
  struct PathNode** children;   // array of pointers to children nodes
  int childCount;               // number of children
} PathNode;

PathNode* createNode(const char* name, bool isFile);

void addPath(PathNode* root, const char* relativePath);

void printTree(PathNode* root);
// Using addPath to add these paths to the tree data structure:
// "src/main.c"
// "src/utils/helpers.c"
// "src/utils/io.c"
// "include/main.h"
// "README.md"
// "docs/usage.txt"
// Then printTree will print to console the directory structure something like:
// 
// ROOT
//  „¥„Ÿsrc
//  | „¥„Ÿ main.c
//  „  „¤„Ÿ utils
//  „     „¥„Ÿ helpers.c
//  „     „¤„Ÿ io.c
//  „¥„Ÿ include
//  „    „¤„Ÿ main.h
//  „¥„Ÿ README.md
//  „¥„Ÿ docs
//  „¤„Ÿ usage.txt
// 
// You can change to something easier / make more sense to you 

void freeTree(PathNode* root);