#include "Archive.h"
#include "Compression.h"
#include "Tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <Windows.h>
#include <stdint.h>

// FUNCTION : createArchive
// DESCRIPTION:
//    Creates a new archive file with the specified name.
//    The archive file is created with an initial header indicating no entries.
// PARAMETERS:
//    const char* fileName : The name of the archive file to create (without extension).
// RETURNS:
//    void
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
// FUNCTION : openArchive
// DESCRIPTION:
//    Opens an existing archive file for reading and writing.
//    The file is expected to have a .dsp extension.
// PARAMETERS:
//    const char* fileName : The name of the archive file to open (without extension).
// RETURNS:
//    FILE* : Pointer to the opened file, or NULL if the file could not be opened.
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
// FUNCTION : closeArchive
// DESCRIPTION:
//    Closes the archive file pointer.
// PARAMETERS:
//    FILE* archivePtr : Pointer to the archive file to close.
// RETURNS:
//    void
void closeArchive(FILE* archivePtr) {
  if (archivePtr != NULL) {
    if (fclose(archivePtr) != 0) {
      perror("Error closing file");
    }
  }
}
// FUNCTION : addFile
// DESCRIPTION:
//    Adds a file to the archive.
//    The file is read in chunks, compressed if necessary, and written to the archive.
//    The file path is stored in the archive header.
//    Each entry is added as a node in the tree structure.
// PARAMETERS:
//    FILE* archivePtr : Pointer to the archive file where the file will be added.
//    const char* filePath : The path of the file to add to the archive.
//    PathNode* root : Pointer to the root of the tree structure for managing paths.
// RETURNS:
//    void
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
// FUNCTION : unpackArchive
// DESCRIPTION:
//    Unpacks the contents of the archive to the specified output directory.
//    It reads the archive header, iterates through each entry, and extracts files or directories.
//    Uncompressed data if the compressed flag is set, and writes it to the output directory.
// PARAMETERS:
//    FILE* archivePtr : Pointer to the archive file to unpack.
//    const char* outputDir : The directory where the unpacked files will be stored.
// RETURNS:
//    void
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
// FUNCTION : addDirectory
// DESCRIPTION:
//    Adds a directory and its contents to the archive.
//    It recursively traverses the directory structure, adding files and subdirectories.
//    Each entry is added as a node in the tree structure.
// PARAMETERS:
//    FILE* archivePtr : Pointer to the archive file where the directory will be added.
//    const char* entryPath : The path of the directory to add to the archive.
//    PathNode* root : Pointer to the root of the tree structure for managing paths.
// RETURNS:
//    int : The number of entries added to the archive (files and directories).
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
  // Update the header at the beginning of the entry
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
// FUNCTION : fileOrDirectoryExists
// DESCRIPTION:
//    Checks if a file or directory exists at the specified path.
// PARAMETERS:
//    const char* filePath : The path to the file or directory to check.
// RETURNS:
//    int : 1 if the file or directory exists, 0 otherwise.
int fileOrDirectoryExists(const char* filePath) {
  DWORD attributes = GetFileAttributesA(filePath);
  return attributes != INVALID_FILE_ATTRIBUTES;
}
// FUNCTION : isFile
// DESCRIPTION:
//    Checks if the specified path is a file.
// PARAMETERS:
//    const char* filePath : The path to the file to check.
// RETURNS:
//    int : 1 if the path is a file, 0 if it is a directory or does not exist.
int isFile(const char* filePath) {
  DWORD attributes = GetFileAttributesA(filePath);
  return attributes != INVALID_FILE_ATTRIBUTES &&
    !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}
// FUNCTION : isDirectory
// DESCRIPTION:
//    Checks if the specified path is a directory.
// PARAMETERS:
//    const char* filePath : The path to the directory to check.
// RETURNS:
//    int : 1 if the path is a directory, 0 if it is a file or does not exist.
int isDirectory(const char* filePath) {
  DWORD attributes = GetFileAttributesA(filePath);
  return attributes != INVALID_FILE_ATTRIBUTES &&
    (attributes & FILE_ATTRIBUTE_DIRECTORY);
}