#include "Archive.h"
#include "Compression.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define CHUNK_SIZE 1024 * 1024 
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

size_t rle_compress(const char* chunk, size_t chunkLength, char* compressedChunk) {
  if (chunkLength == 0) {
    return 0; // nothing to compress
  }

  char* out = compressedChunk;
  size_t outSize = 0;

  for (size_t i = 0; i < chunkLength; ) {
    unsigned char count = 1;
    char value = chunk[i];

    // Count run length (max 255)
    while (i + count < chunkLength && chunk[i + count] == value && count < 255) {
      count++;
    }

    // Output run
    outSize += 2;
    if (outSize >= chunkLength * 2) {
      return 0;
    }
    *out++ = count;
    *out++ = value;

    i += count;
  }

  return outSize;
}

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
  char *buffer = malloc(CHUNK_SIZE);
  char* compressedBuffer = malloc(CHUNK_SIZE * 4);

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
    size_t compressedSize = lz77_compress(buffer, bytesRead, compressedBuffer);
    printf("Read %u bytes, Compressed to %zu bytes\n", bytesRead, compressedSize);
    if (compressedSize == 0) {
      byteWritten += bytesRead + 4;
      fwrite(&bytesRead, sizeof(bytesRead), 1, archivePointer);
      fwrite(buffer, 1, bytesRead, archivePointer);
    } 
    else {
      byteWritten += compressedSize + 4;
      fwrite(&compressedSize, sizeof(compressedSize), 1, archivePointer);
      fwrite(compressedBuffer, 1, compressedSize, archivePointer);
    }
    
  }
  free(buffer);
  free(compressedBuffer);
  
  strcpy_s(header.path, sizeof(header.path), entryPath);
  header.isFile = 1; 
  header.isCompressed = 0; 
  header.isEncrypted = 0; 
  header.originalSize = byteReadTotal;
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