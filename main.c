#include <Windows.h>
#include <sys/stat.h>
#include <stdio.h> 
#include "Archive.h"

int main(void) {
  addFile("test_archive.dsp", "Test.txt"); 
  addFile("test_archive.dsp", "Test2.txt");
}