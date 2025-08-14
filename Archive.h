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
void addFile(const char* archive, const char* entryPath);
size_t rle_compress(const char* chunk, size_t chunkLength, char* compressedChunk);