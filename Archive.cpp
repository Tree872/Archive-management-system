#include "Archive.h"
#include "Compression.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <Windows.h>
#include <sys/stat.h>
#include <stdint.h>

#define CHUNK_SIZE 1024 * 1024

void createArchive(const char* fileName) {
  char fileFullName[256];
  sprintf_s(fileFullName, sizeof(fileFullName), "%s.dsp", fileName);
  FILE* filePointer;
  errno_t err = fopen_s(&filePointer, fileFullName, "wb");
  if (filePointer == NULL || err != 0) {
    perror("Error opening file");
    return;
  }
  ArchiveHeader header;
  header.entryCount = 0;
  fwrite(&header, sizeof(header), 1, filePointer);
  if (fclose(filePointer) != 0) {
    perror("Error closing file");
    return;
  }
}

FILE* openArchive(const char* fileName) {
  char fileFullName[256];
  sprintf_s(fileFullName, sizeof(fileFullName), "%s.dsp", fileName);
  FILE* filePointer;
  errno_t err = fopen_s(&filePointer, fileFullName, "rb+");
  if (filePointer == NULL || err != 0) {
    perror("Error opening file");
    return NULL;
  }
  return filePointer;
}
void closeArchive(FILE* archivePtr) {
  if (archivePtr != NULL) {
    if (fclose(archivePtr) != 0) {
      perror("Error closing file");
    }
  }
}

void addFile(FILE* archivePtr, const char* filePath) {
  if (archivePtr == NULL) {
    perror("Error: Archive pointer is NULL");
    return;
  }
  fseek(archivePtr, 0, SEEK_END); // Move to the end of the file
  size_t headerPos = _ftelli64(archivePtr);
  EntryHeader header;
  memset(&header, 0, sizeof(header)); // Initialize header to zero
  // Writing an empty header initially
  fwrite(&header, sizeof(header), 1, archivePtr);

  FILE* entryPointer;
  errno_t err = fopen_s(&entryPointer, filePath, "rb");
  if (entryPointer == NULL || err != 0) {
    perror("Error opening entry file");
    printf("%s\n", filePath);
    return;
  }
  char *buffer = (char*)malloc(CHUNK_SIZE);
  char* compressedBuffer = (char*)malloc(CHUNK_SIZE);

  if (buffer == NULL || compressedBuffer == NULL) {
    free(buffer);
    free(compressedBuffer);
    return;
  }
  size_t byteWritten = 0;
  size_t byteReadTotal = 0;
  unsigned int bytesRead;
  while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, entryPointer)) > 0) {
    byteReadTotal += bytesRead;
    if (1) { // No compression for now
      byteWritten += bytesRead + 4;
      fwrite(&bytesRead, sizeof(bytesRead), 1, archivePtr);
      fwrite(buffer, 1, bytesRead, archivePtr);
    } 
    
  }
  free(buffer);
  free(compressedBuffer);
  
  strcpy_s(header.path, sizeof(header.path), filePath);
  header.isFile = 1; 
  header.isCompressed = 0; 
  header.isEncrypted = 0; 
  header.originalSize = byteReadTotal;
  header.storedSize = byteWritten;
  // Update the header at the beginning of the archive
  fseek(archivePtr, headerPos, SEEK_SET);
  fwrite(&header, sizeof(header), 1, archivePtr);
  // Update the entry count in the archive header
  fseek(archivePtr, 0, SEEK_SET);
  ArchiveHeader archiveHeader;
  fread(&archiveHeader, sizeof(archiveHeader), 1, archivePtr); // Read existing header
  archiveHeader.entryCount++;
  fseek(archivePtr, 0, SEEK_SET);
  fwrite(&archiveHeader, sizeof(archiveHeader), 1, archivePtr); // Write updated header

  if (fclose(entryPointer) != 0) {
    perror("Error closing entry file");
    return;
  }

}

int addDirectory(FILE* archivePtr, const char* entryPath) {
  int readEntries = 0;
  if (archivePtr == NULL) {
    perror("Error: Archive pointer is NULL");
    return readEntries;
  }

  fseek(archivePtr, 0, SEEK_END); // Move to the end of the file

  size_t headerPos = _ftelli64(archivePtr);
  // Update the header at the beginning of the archive
  EntryHeader header;
  strcpy_s(header.path, sizeof(header.path), entryPath);
  header.isFile = 0;
  header.isCompressed = 0;
  header.isEncrypted = 0;
  header.originalSize = 0;
  header.storedSize = 0;
  fseek(archivePtr, headerPos, SEEK_SET);
  fwrite(&header, sizeof(header), 1, archivePtr);
  LPWIN32_FIND_DATAA findFileData = (LPWIN32_FIND_DATAA)malloc(sizeof(WIN32_FIND_DATAA));
  if (findFileData == NULL) {
    perror("Error allocating memory for find data");
    return readEntries;
  }
  HANDLE hFind = INVALID_HANDLE_VALUE;
  char searchPath[512];
  snprintf(searchPath, sizeof(searchPath), "%s\\*", entryPath);
  printf("Searching in: %s\n", searchPath);
  hFind = FindFirstFileA(searchPath, findFileData);
  if (hFind == INVALID_HANDLE_VALUE) {
    perror("Error opening directory");
    return readEntries;
  }
  // Loop through all files and directories in the specified directory
  do {
    if (strcmp(findFileData->cFileName, ".") != 0 && strcmp(findFileData->cFileName, "..") != 0) {
      char fullPath[512];
      snprintf(fullPath, sizeof(fullPath), "%s\\%s", entryPath, (char*)findFileData->cFileName);
      if (findFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // It's a directory, recursively add it
        readEntries += addDirectory(archivePtr, fullPath);
      } else {
        // It's a file, add it to the archive
        addFile(archivePtr, fullPath);
        readEntries++;
      }
    }
  } while (FindNextFileA(hFind, findFileData) != 0);
  free(findFileData);

  FindClose(hFind);
  // Update the entry count in the archive header
  fseek(archivePtr, 0, SEEK_SET);
  ArchiveHeader archiveHeader;
  fread(&archiveHeader, sizeof(archiveHeader), 1, archivePtr); // Read existing header
  archiveHeader.entryCount++;
  fseek(archivePtr, 0, SEEK_SET);
  fwrite(&archiveHeader, sizeof(archiveHeader), 1, archivePtr); // Write updated header
  return readEntries + 1;
}