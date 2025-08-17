#ifndef CONSOLE_IO_H
#define CONSOLE_IO_H

#include <stdbool.h>

#define TREE_BRANCH_MID "+-- "
#define TREE_VERTICAL   "|   "
#define TREE_BRANCH_END "`-- "

typedef struct PathNode {
    char relativePath[256];     // name of this file or folder
    bool isFile;                // true if file, false if directory
    struct PathNode** children; // array of pointers to children nodes
	int childCount;             // number of children
} PathNode;

PathNode* createNode(const char* name, bool isFile);
void addPath(PathNode* root, const char* relativePath);
void printTree(PathNode* root);
void freeTree(PathNode* root);

#endif
