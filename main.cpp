#include <Windows.h>
#include <sys/stat.h>
#include <stdio.h> 
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <stdio.h>
#include "Archive.h"
#include "Compression.h"

void printBinary(const char data) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", (data >> i) & 1);
  }
  printf(" ");
}

int main() {
  /*createArchive("test_archive");
  FILE* archivePtr = openArchive("test_archive");
  printf("%d\n", addDirectory(archivePtr, "TestDir"));
  closeArchive(archivePtr);*/
  
  const char* input = "123451234512345";
  char output[1024];
  int compressedSize = lzssEncode(input, strlen((const char*)input), output);
  printf("Original size: %d, compressed size: %d\n", strlen(input), compressedSize);
  printf("Compressed data: \n");
  for (int i = 0; i < compressedSize; i++) {
    printBinary(output[i]);
    printf("%c\n", output[i]);
  }
  printf("\n");

  return 0;
}
