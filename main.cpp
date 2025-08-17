#include <Windows.h>
#include <sys/stat.h>
#include <stdio.h> 
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include "Archive.h"
#include "Compression.h"

void printBinary(const char data) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", (data >> i) & 1);
  }
  printf(" ");
}

int main() {
  createArchive("test_archive");
  FILE* archivePtr = openArchive("test_archive");
  addFile(archivePtr, "Galaxy.dll");
  closeArchive(archivePtr);

  
PathNode* root = createNode("ROOT", false);

addPath(root, "src/main.c");
addPath(root, "src/utils/helpers.c");
addPath(root, "src/utils/io.c");
addPath(root, "include/main.h");
addPath(root, "README.md");
addPath(root, "docs/usage.txt");

printTree(root);
freeTree(root);

  return 0;
}
