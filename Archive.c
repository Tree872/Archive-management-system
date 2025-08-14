#include "Archive.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define CHUNK_SIZE 1048576 // 1 MB
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void createArchive(const char* fileName) {
  char fileFullName[256];
  sprintf_s(fileFullName, sizeof(fileFullName), "%s.dsp", fileName);
  FILE* filePointer;
  errno_t err = fopen_s(&filePointer, fileFullName, "wb");
  if (filePointer == NULL || err != 0) {
    perror("Error opening file");
    return;
  }
  if (fclose(filePointer) != 0) {
    perror("Error closing file");
    return;
  }
}

void addFile(const char* archive, const char* entryPath) {
  FILE* archivePointer;
  errno_t err = fopen_s(&archivePointer, archive, "rb+");
  if (archivePointer == NULL || err != 0) {
    perror("Error opening archive file");
    return;
  }
  fseek(archivePointer, 0, SEEK_END); // Move to the end of the file
  size_t headerPos = _ftelli64(archivePointer);
  EntryHeader header;
  memset(&header, 0, sizeof(header)); // Initialize header to zero
  // Writing an empty header initially
  fwrite(&header, sizeof(header), 1, archivePointer);

  FILE* entryPointer;
  err = fopen_s(&entryPointer, entryPath, "rb");
  if (entryPointer == NULL || err != 0) {
    perror("Error opening entry file");
    fclose(archivePointer);
    return;
  }
  char buffer[CHUNK_SIZE];
  size_t byteWritten = 0;
  size_t bytesRead;
  while ((bytesRead = fread(buffer, 1, sizeof(buffer), entryPointer)) > 0) {
    byteWritten += bytesRead;
    fwrite(buffer, 1, bytesRead, archivePointer);
  }
  
  strcpy_s(header.path, sizeof(header.path), entryPath);
  header.isFile = 1; 
  header.isCompressed = 0; 
  header.isEncrypted = 0; 
  header.originalSize = byteWritten;
  header.storedSize = byteWritten;
  // Update the header at the beginning of the archive
  fseek(archivePointer, headerPos, SEEK_SET);
  fwrite(&header, sizeof(header), 1, archivePointer);

  if (fclose(archivePointer) != 0) {
    perror("Error closing archive file");
    return;
  }
  if (fclose(entryPointer) != 0) {
    perror("Error closing entry file");
    return;
  }

}