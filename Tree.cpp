#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Tree.h"

#define _CRT_SECURE_NO_WARNINGS

PathNode* createNode(const char* name, bool isFile) {
  PathNode* node = (PathNode*)malloc(sizeof(PathNode));
  if (!node) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  strncpy_s(node->relativePath, sizeof(node->relativePath), name, _TRUNCATE);

  node->isFile = isFile;
  node->children = NULL;
  node->childCount = 0;
  return node;
}

static void addPathRecursive(PathNode* current, const char* path) {
  // Make a local copy so we can modify safely
  char buffer[512];

  strncpy_s(buffer, sizeof(buffer), path, _TRUNCATE);

  // Split path on '\'
  char* slash = strchr(buffer, '\\');
  bool isFile = (slash == NULL);

  if (slash) *slash = '\0';  // terminate first part

  // Search for existing child
  PathNode* child = NULL;
  for (int i = 0; i < current->childCount; i++) {
    if (strcmp(current->children[i]->relativePath, buffer) == 0) {
      child = current->children[i];
      break;
    }
  }

  // If not found, create new child
  if (!child) {
    child = createNode(buffer, isFile);
    current->children = (PathNode**)realloc(
      current->children, sizeof(PathNode*) * (current->childCount + 1)
    );
    if (!current->children) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }
    current->children[current->childCount++] = child;
  }

  // If there is more path after slash, recurse
  if (!isFile && slash) {
    addPathRecursive(child, slash + 1);
  }
}

void addPath(PathNode* root, const char* relativePath) {
  addPathRecursive(root, relativePath);
}

static void printTreeRecursive(PathNode* node, const char* prefix, bool isLast) {
  if (strcmp(node->relativePath, "ROOT") != 0) {
    printf("%s%s%s\n",
      prefix,
      isLast ? TREE_BRANCH_END : TREE_BRANCH_MID,
      node->relativePath);
  }

  char newPrefix[1024];
  snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : TREE_VERTICAL);

  for (int i = 0; i < node->childCount; i++) {
    printTreeRecursive(node->children[i], newPrefix, i == node->childCount - 1);
  }
}

void printTree(PathNode* root) {
  printf("ROOT\n");
  for (int i = 0; i < root->childCount; i++) {
    printTreeRecursive(root->children[i], "", i == root->childCount - 1);
  }
}

void freeTree(PathNode* root) {
  for (int i = 0; i < root->childCount; i++) {
    freeTree(root->children[i]);
  }
  free(root->children);
  free(root);
}