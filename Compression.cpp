#include "Compression.h"
#include "External\robin_hood.h" // For hash table
#include <stdint.h>

// Set the nth bit of x (leftmost bit is 0)
#define SET_BIT(x, n) ((x) | (0b10000000 >> (n)))
// Check if the nth bit of x is set (leftmost bit is 0)
#define CHECK_BIT(x, n) ((x) & (0b10000000 >> (n)))
// FUNCTION : packToken
// DESCRIPTION:
//   Packs the offset and length into a single 16-bit token.
// PARAMETERS:
//   uint16_t offset : The offset of the match in the sliding window.
//   uint8_t length : The length of the match.
// RETURNS:
//   LZSSPackedToken : The packed token containing offset and length.
LZSSPackedToken packToken(uint16_t offset, uint8_t length) {
  return (((offset - 1) & 0x0FFF) << 4) | ((length - MIN_MATCH) & 0x0F);
}
// FUNCTION : unpackToken
// DESCRIPTION:
//    Unpacks the packed token into offset and length.
// PARAMETERS:
//    LZSSPackedToken packed : The packed token to unpack.
//    uint16_t* offset : Pointer to store the unpacked offset.
//    uint8_t* length : Pointer to store the unpacked length.
// RETURNS:
//    void
void unpackToken(LZSSPackedToken packed, uint16_t* offset, uint8_t* length) {
  *offset = ((packed >> 4) & 0x0FFF) + 1;
  *length = (packed & 0x0F) + MIN_MATCH;
}
// FUNCTION : lzssEncode
// DESCRIPTION:
//    Encodes the input data using LZSS compression algorithm.
//    This function assumes that the output buffer is large enough to hold the compressed data. (1.25x at least)
// PARAMETERS:
//    const char* input : Pointer to the input data to compress.
//    int inputSize : Size of the input data in bytes.
//    char* output : Pointer to the output buffer where compressed data will be stored.
// RETURNS:
//    int : The size of the compressed data in bytes, or -1 if compression was not possible.
int lzssEncode(const char* input, int inputSize, char* output) {
  if (inputSize < MIN_MATCH * 2) { // Not enough data to compress
    return -1; // Return -1 to indicate no compression 
  }

  int slidingWindowStart = 0;
  int slidingWindowEnd = MIN_MATCH - 1; 
  int outputSize = 4; // Will be used as index for output
  int flagCounter = 3; // Track bits for the flag byte 
  int flagPos = 0; // Position in the output to be reserved for the flag byte
  char flagByte = 0x00; // The flag byte itself to indicate literals and matches

  // Key: 3-byte substring, Value: list of start indices
  robin_hood::unordered_map<uint32_t, IndexQueue> substringIndexMap;
  IndexQueue initialQueue;
  initIndexQueue(&initialQueue);
  // Initialize with the first substring at index 0
  pushIndexQueue(&initialQueue, 0);
  substringIndexMap.insert({packToUInt32(input), initialQueue});

  // Initialize the output with the first MIN_MATCH bytes as literals
  for (int i = 1; i <= MIN_MATCH; i++) {
    output[i] = input[i - 1];
  }

  // For each byte in the input
  for (int i = MIN_MATCH; i < inputSize; i++) {
    uint32_t currentSubstring;    
    currentSubstring = packToUInt32(input + i);

    int longestMatchLength = 1; // Equal 1 when no match is found
    int longestMatchOffset = 0;
    // Check if the current substring exists in the sliding window
    if (substringIndexMap.contains(currentSubstring) && i + MIN_MATCH <= inputSize) {
      
      int headQueue = substringIndexMap[currentSubstring].head;
      int tailQueue = substringIndexMap[currentSubstring].tail;

      // For each starting index of the current substring in the sliding window
      for (int j = 0; j < substringIndexMap[currentSubstring].length; j++) {
        int queueIndex = (headQueue + j) % MAX_OCCURRENCE_PER_SUBSTRING;
        int startIndex = substringIndexMap[currentSubstring].buffer[queueIndex];
        int matchLength = MIN_MATCH;
        // Check how long the match continues
        while (1) {
          int currentIndex = startIndex + matchLength; // Index in the slidng window
          int lookaheadIndex = i + matchLength; // Index in the input

          if (currentIndex <= slidingWindowEnd &&
            lookaheadIndex < inputSize &&
            matchLength < MAX_MATCH &&
            input[currentIndex] == input[lookaheadIndex]) {
            matchLength++;
          }
          else {
            break; // No longer matching
          }
        }

        if (matchLength > longestMatchLength) {
          longestMatchLength = matchLength;
          longestMatchOffset = i - startIndex;
        }

      }

      // Pack the token and write it to output
      LZSSPackedToken packedToken = packToken(longestMatchOffset, longestMatchLength);
      output[outputSize] = packedToken >> 8; // First byte
      outputSize++;
      output[outputSize] = packedToken & 0xFF; // Second byte
      outputSize++;
      flagByte = SET_BIT(flagByte, flagCounter);
      flagCounter++;
      i += longestMatchLength - 1; // Adjust i to skip over the matched bytes
    }
    else {
      // No match found, write the literal byte
      output[outputSize] = input[i];
      outputSize++;
      flagCounter++;
    }

    if (flagCounter == 8 || i == inputSize - 1) {
      // If we have 8 flags or reached the end of input, write the flag byte
      output[flagPos] = flagByte;
      flagPos = outputSize; // Update flag position for next byte
      flagByte = 0x00; // Reset the flag byte
      flagCounter = 0; // Reset the counter
      if (i < inputSize - 1) {
        outputSize++; // Don't increment if at the end of input
      }
    }
    // Adding new substrings to the sliding window
    for (int j = 0; j < longestMatchLength; j++) {
      slidingWindowEnd++;
      int newIndex = slidingWindowEnd - MIN_MATCH + 1;
      uint32_t newSubstring = packToUInt32(input + newIndex);

      if (!substringIndexMap.contains(newSubstring)) {
        // If the substring is not already in the map, create a new queue
        IndexQueue newIndexQueue;
        initIndexQueue(&newIndexQueue);
        substringIndexMap[newSubstring] = newIndexQueue;
      }
      // Add the new index to the queue for this substring
      pushIndexQueue(&substringIndexMap[newSubstring], newIndex);
    }
    
    // Remove oldest substrings if the sliding window exceeds the size
    while (slidingWindowEnd - slidingWindowStart + 1 > WINDOW_SIZE) {
      uint32_t oldestSubstring = packToUInt32(input + slidingWindowStart);
      if (substringIndexMap.contains(oldestSubstring)) {
        IndexQueue &indices = substringIndexMap[oldestSubstring];
        // Remove from the front
        popIndexQueue(&indices);
        if (indices.length < 1) {
          substringIndexMap.erase(oldestSubstring); // Remove the entry if no indices left
        }
 
      }
      slidingWindowStart++;
    }
  }
  return outputSize;
}
// FUNCTION : lzssDecode
// DESCRIPTION:
//    Decodes the input data using LZSS decompression algorithm.
//    This function assumes that the output buffer is large enough to hold the decompressed data.
//    It also assumes that the input data is valid and follows the LZSS format.
// PARAMETERS:
//    const char* input : Pointer to the compressed data to decode.
//    int inputSize : Size of the compressed data in bytes.
//    char* output : Pointer to the output buffer where decompressed data will be stored.
// RETURNS:
//    int : The size of the decompressed data in bytes, or 0 if no data was decoded.
int lzssDecode(const char* input, int inputSize, char* output) {
  if (inputSize < 1) {
    return 0; // No data to decode
  }
  int outputSize = 0;
  int inputIndex = 1;
  char flagByte = input[0];
  int flagCounter = 0;
  while (inputIndex < inputSize) {
    if (flagCounter == 8) {
      // Read the next flag byte
      flagByte = input[inputIndex];
      inputIndex++;
      flagCounter = 0;
    }
    if (CHECK_BIT(flagByte, flagCounter)) {
      // It's a match token
      if (inputIndex + 1 >= inputSize) {
        break; // Not enough data for a token
      }
      char firstByte = input[inputIndex];
      char secondByte = input[inputIndex + 1];
      LZSSPackedToken packedToken = (firstByte << 8) | (secondByte & 0xFF);
      inputIndex += 2;
      uint16_t offset = 0;
      uint8_t length = 0;
      
      unpackToken(packedToken, &offset, &length);

      // Copy the matched string from the output buffer
      for (int i = 0; i < length; i++) {
        output[outputSize] = output[outputSize - offset];
        outputSize++;
      }
    } 
    else {
      // It's a literal byte
      if (inputIndex >= inputSize) {
        break; // No more data
      }
      output[outputSize] = input[inputIndex];
      inputIndex++;
      outputSize++;
    }
    flagCounter++;
  }
  return outputSize;
}
// FUNCTION : packToUInt32
// DESCRIPTION:
//    Packs the first 3 bytes from the pointer location.
//    This function assumes that from the pointer there are at least 3 bytes of valid data.
//    Used to extract a substring from a big string.
// PARAMETERS:
//    const char* data : Pointer to the input current position in the string containing at least 3 bytes.
// RETURNS:
//    uint32_t : The packed 32-bit unsigned integer containing the first 3 bytes of data.
uint32_t packToUInt32(const char* data) {
  uint32_t packedUint32 = 0x0000;
  for (int i = 0; i < MIN_MATCH; i++) {
    packedUint32 = (packedUint32 << 8) | (unsigned char)(data[i]);
  }
  return packedUint32;
}
// FUNCTION : popIndexQueue
// DESCRIPTION:
//    Pops the oldest index from the queue.
// PARAMETERS:
//    IndexQueue* queue : Pointer to the IndexQueue from which to pop the index.
// RETURNS:
//    int : The popped index, or -1 if the queue is empty.
int popIndexQueue(IndexQueue* queue) {
  if (queue->length == 0) {
    return -1; // Queue is empty
  }
  int index = queue->buffer[queue->head];
  queue->head = (queue->head + 1) % MAX_OCCURRENCE_PER_SUBSTRING;
  queue->length--;
  return index;
}
// FUNCTION : pushIndexQueue
// DESCRIPTION:
//    Pushes a new index to the queue, overwriting the oldest if the queue is full.
// PARAMETERS:
//    IndexQueue* queue : Pointer to the IndexQueue to which to push the index.
//    int index : The index to push into the queue.
// RETURNS:
//    void
void pushIndexQueue(IndexQueue* queue, int index) {
  if (queue->length == MAX_OCCURRENCE_PER_SUBSTRING) {
    // Queue is full - overwrite oldest element (move head forward)
    queue->head = (queue->head + 1) % MAX_OCCURRENCE_PER_SUBSTRING;
    // length stays the same since we're replacing, not adding
  }
  else {
    // Queue has space - increment length
    queue->length++;
  }

  queue->buffer[queue->tail] = index;
  queue->tail = (queue->tail + 1) % MAX_OCCURRENCE_PER_SUBSTRING;
}
// FUNCTION : initIndexQueue
// DESCRIPTION:
//    Initializes the IndexQueue to an empty state.
// PARAMETERS:
//    IndexQueue* queue : Pointer to the IndexQueue to initialize.
// RETURNS:
//    void
void initIndexQueue(IndexQueue* queue) {
  queue->head = 0;
  queue->tail = 0;
  queue->length = 0;
  for (int i = 0; i < MAX_OCCURRENCE_PER_SUBSTRING; i++) {
    queue->buffer[i] = -1; // Initialize with -1
  }
  return;
}