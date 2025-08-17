#include <Windows.h>
#include <sys/stat.h>
#include <stdio.h> 
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <string>
#include "Archive.h"
#include "ConsoleIO.h"

int main() {
  PathNode* root = createNode("root", false);

 
  createArchive("test_archive");
  FILE* archivePtr = openArchive("test_archive");
  addDirectory(archivePtr, "Update", root);
  unpackArchive(archivePtr, "test_archive");
  closeArchive(archivePtr);

  

  printTree(root);
  freeTree(root);
  return 0;
}
