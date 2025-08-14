#include <Windows.h>
#include <sys/stat.h>
#include <stdio.h> 
#include <string.h>
#include "Archive.h"

int main(void) {
  //createArchive("test_archive");
  //addFile("test_archive.dsp", "book.txt");
  char testString[100] = "„¤\0";
  printf("%d\n", (int)strlen(testString));
  printf("„¥„Ÿ ");
  printf("„¤„Ÿ ");

}