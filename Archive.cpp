#include "Archive.h"
#include "Compression.h"
#include "Tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <Windows.h>
#include <stdint.h>

#define CHUNK_SIZE 32768 // 32 KB

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

void addFile(FILE* archivePtr, const char* filePath, PathNode* root) {
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
    return;
  }
  char *buffer = (char*)malloc(CHUNK_SIZE);
  char *compressedBuffer = (char*)malloc(CHUNK_SIZE * 2);

  if (buffer == NULL || compressedBuffer == NULL) {
    free(buffer);
    free(compressedBuffer);
    return;
  }

  int toCompress = 1;
  int inARow = 0; 
  size_t byteWritten = 0;
  size_t byteReadTotal = 0;
  char chunkCompressedFlag = 0; // Flag to indicate if the chunk is compressed
  int bytesRead;
  while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, entryPointer)) > 0) {
    byteReadTotal += bytesRead;
    int compressedSize = 0;

    if (toCompress) {
      compressedSize = lzssEncode(buffer, bytesRead, compressedBuffer);
    }
    else {
      compressedSize = bytesRead + 1; 
    }

    if (compressedSize > bytesRead) { // If compression is not effective
      inARow++;
      if (inARow >= 10) { 
        toCompress = 0; // Switch to not compressing
      }
      chunkCompressedFlag = 0; // Not compressed
    } 
    else {
      chunkCompressedFlag = 1; // Compressed
      inARow = 0; 
    }
    fwrite(&chunkCompressedFlag, sizeof(chunkCompressedFlag), 1, archivePtr); // Write chunk flag
    
    if (chunkCompressedFlag == 0) {
      // If not compressed, write the original data
      fwrite(&bytesRead, sizeof(bytesRead), 1, archivePtr); // Write chunk prefix
      fwrite(buffer, 1, bytesRead, archivePtr);
      byteWritten += bytesRead + sizeof(compressedSize) + sizeof(chunkCompressedFlag);
    }
    else {
      fwrite(&compressedSize, sizeof(compressedSize), 1, archivePtr); // Write chunk prefix
      fwrite(compressedBuffer, 1, compressedSize, archivePtr);
      byteWritten += compressedSize + sizeof(compressedSize) + sizeof(chunkCompressedFlag);
    }

  }
  free(buffer);
  free(compressedBuffer);
  
  strcpy_s(header.path, sizeof(header.path), filePath);
  header.isFile = 1;
  header.isCompressed = 1;
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
  addPath(root, filePath); // Add the file to the tree structure

  if (fclose(entryPointer) != 0) {
    perror("Error closing entry file");
    return;
  }
}

void unpackArchive(FILE* archivePtr, const char* outputDir) {
  if (archivePtr == NULL) {
    perror("Error: Archive pointer is NULL");
    return;
  }
  fseek(archivePtr, 0, SEEK_SET); // Move to the beginning of the file
  ArchiveHeader archiveHeader;
  fread(&archiveHeader, sizeof(archiveHeader), 1, archivePtr);
  CreateDirectoryA(outputDir, NULL); // Create output directory if it doesn't exist
  char* readBuffer = (char*)malloc(CHUNK_SIZE * 2);
  char* writeBuffer = (char*)malloc(CHUNK_SIZE);
  for (unsigned long long i = 0; i < archiveHeader.entryCount; i++) {
    EntryHeader entryHeader;
    fread(&entryHeader, sizeof(entryHeader), 1, archivePtr);

    char fullOutputPath[512];
    snprintf(fullOutputPath, sizeof(fullOutputPath), "%s\\%s", outputDir, entryHeader.path);
    
    if (entryHeader.isFile) {
      FILE* outputFile;
      errno_t err = fopen_s(&outputFile, fullOutputPath, "wb");
      if (err != 0 || outputFile == NULL) {
        perror("Error creating output file");
        continue;
      }

      if (readBuffer == NULL || writeBuffer == NULL) {
        fclose(outputFile);
        continue;
      }

      size_t bytesRead = 0;
      while (bytesRead < entryHeader.storedSize) {
        int chunkSize = 0;
        char chunkCompressedFlag = 0;
        fread(&chunkCompressedFlag, sizeof(chunkCompressedFlag), 1, archivePtr);
        fread(&chunkSize, sizeof(chunkSize), 1, archivePtr);
        bytesRead += sizeof(chunkSize) + sizeof(chunkCompressedFlag);
        bytesRead += fread(readBuffer, 1, chunkSize, archivePtr);
        if (chunkCompressedFlag == 0) {
          // If not compressed, write the original data
          fwrite(readBuffer, 1, chunkSize, outputFile);
        } 
        else if (chunkCompressedFlag == 1) {
          // If compressed, decompress the data
          int decompressedSize = lzssDecode(readBuffer, chunkSize, writeBuffer);
          fwrite(writeBuffer, 1, decompressedSize, outputFile);
        }
        
      }
      fclose(outputFile);
    } 
    else {
      // Handle directories
      CreateDirectoryA(fullOutputPath, NULL);
    }
   
  }
  free(readBuffer);
  free(writeBuffer);
}

int addDirectory(FILE* archivePtr, const char* entryPath, PathNode* root) {
  int readEntries = 0;
  if (archivePtr == NULL) {
    perror("Error: Archive pointer is NULL");
    return readEntries;
  }

  
  LPWIN32_FIND_DATAA findFileData = (LPWIN32_FIND_DATAA)malloc(sizeof(WIN32_FIND_DATAA));
  if (findFileData == NULL) {
    perror("Error allocating memory for find data");
    return readEntries;
  }
  HANDLE hFind = INVALID_HANDLE_VALUE;
  char searchPath[512];
  snprintf(searchPath, sizeof(searchPath), "%s\\*", entryPath);
  hFind = FindFirstFileA(searchPath, findFileData);
  if (hFind == INVALID_HANDLE_VALUE) {
    perror("Error opening directory");
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
  addPath(root, entryPath); // Add the directory to the tree structure
  // Loop through all files and directories in the specified directory
  while (1) {
    if (strcmp(findFileData->cFileName, ".") != 0 && strcmp(findFileData->cFileName, "..") != 0) {
      char fullPath[512];
      snprintf(fullPath, sizeof(fullPath), "%s\\%s", entryPath, (char*)findFileData->cFileName);
      if (isFile(fullPath)) {
        // It's a file, add it to the archive
        addFile(archivePtr, fullPath, root);
        readEntries++;
      } else {
        // It's a directory, recursively add it
        readEntries += addDirectory(archivePtr, fullPath, root);
      }
    }
    if (FindNextFileA(hFind, findFileData) == 0) {
      break;
    }
  }

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

int fileOrDirectoryExists(const char* filePath) {
  DWORD attributes = GetFileAttributesA(filePath);
  return attributes != INVALID_FILE_ATTRIBUTES;
}

int isFile(const char* filePath) {
  DWORD attributes = GetFileAttributesA(filePath);
  return attributes != INVALID_FILE_ATTRIBUTES &&
    !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

int isDirectory(const char* filePath) {
  DWORD attributes = GetFileAttributesA(filePath);
  return attributes != INVALID_FILE_ATTRIBUTES &&
    (attributes & FILE_ATTRIBUTE_DIRECTORY);
}