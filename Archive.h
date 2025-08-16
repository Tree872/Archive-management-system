#pragma once
#include <stdio.h>

typedef struct {
  char path[256];
  unsigned long long originalSize;
  unsigned long long storedSize;
  unsigned char isFile;
  unsigned char isCompressed;
  unsigned char isEncrypted; 
} EntryHeader;

typedef struct {
  unsigned long long entryCount;
} ArchiveHeader;


void createArchive(const char* fileName);
FILE* openArchive(const char* fileName);
void addFile(FILE* archivePtr, const char* entryPath);
int addDirectory(FILE* archivePtr, const char* entryPath);
void closeArchive(FILE* archivePtr);