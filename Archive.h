#pragma once
#include "Tree.h"
#include <stdio.h>

#define CHUNK_SIZE 32768 // 32 KB

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
void addFile(FILE* archivePtr, const char* entryPath, PathNode* root);
int addDirectory(FILE* archivePtr, const char* entryPath, PathNode* root);
void unpackArchive(FILE* archivePtr, const char* outputDir);
void closeArchive(FILE* archivePtr);

int fileOrDirectoryExists(const char* path);
int isFile(const char* path);
int isDirectory(const char* path);