#include "Compression.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
typedef uint16_t LZ77PackedToken;
// Set the nth bit of x (leftmost bit is 0)
#define SET_BIT(x, n) ((x) | (0b10000000 >> (n)))

#define MIN_MATCH 3
#define MAX_MATCH 15 + MIN_MATCH 
#define WINDOW_SIZE 4096

uint16_t packToken(uint16_t offset, uint8_t length) {
  return (offset << 4) | ((length - MIN_MATCH) & 0xFF);
}

void unpackToken(uint16_t packed, uint16_t* offset, uint8_t* length) {
  *offset = packed >> 4;
  *length = (packed & 0xFF) + MIN_MATCH; 
}

int lzssEncode(const char* input, int inputSize, char* output) {
  if (inputSize < MIN_MATCH * 2) { // Not enough data to compress
    output[0] = 0x00; // No compression flag
    memcpy(output + 1, input, inputSize);
    return inputSize + 1; // Return original size + flag byte
  }

  int slidingWindowStart = 0;
  int slidingWindowEnd = MIN_MATCH - 1; 
  int outputSize = 4; // Will be used as index for output
  // This variable cycles through 0 to 7, representing the bits in the current byte
  int flagCounter = 3; // Start with 3 since the first 3 bytes are always literals
  int flagPos = 0; // Position in the output for current the flag byte
  char flagByte = 0x00; // The flag byte itself to indicate literals and matches
  // Key: 3-byte substring, Value: list of start indices
  std::unordered_map<std::string, std::vector<int>> windowSlide; 
  // Initialize with the first MIN_MATCH bytes
  windowSlide.insert({std::string(input, MIN_MATCH), {0}});

  for (int i = 1; i <= MIN_MATCH; i++) {
    // Initialize the output with the first MIN_MATCH bytes as literals
    output[i] = input[i - 1];
  }
  // For each byte in the input
  for (int i = MIN_MATCH; i < inputSize; i++) {
    std::cout << slidingWindowStart << " " << slidingWindowEnd << std::endl;
    std::cout << "Current window: " << std::string(input + slidingWindowStart, slidingWindowEnd - slidingWindowStart + 1) << std::endl;
    std::string currentSubstring;
    if (i + MIN_MATCH <= inputSize) {     
      currentSubstring.assign(input + i, MIN_MATCH);
    }
    else {
      // If we are at the end of the input, we can't form a full MIN_MATCH substring
      currentSubstring = ""; // Set to empty so it doesn't match anything
    }
    int longestMatchLength = 1;
    int longestMatchOffset = 0;
    // Check if the current substring exists in the sliding window
    if (windowSlide.find(currentSubstring) != windowSlide.end()) {
      // Found a match, now find the longest match
      std::cout << "Found match for: " << currentSubstring << std::endl;
      std::cout << "Indices in sliding window: ";
      for (int index : windowSlide[currentSubstring]) {
        std::cout << index << " ";
      }
      // For each starting index of the current substring in the sliding window
      for (int startIndex : windowSlide[currentSubstring]) {
        int matchLength = MIN_MATCH;
        // Check how long the match continues
        while (1) {
          int currentIndex = startIndex + matchLength;
          if (currentIndex <= slidingWindowEnd &&
              matchLength < MAX_MATCH && i + matchLength < inputSize &&
              input[currentIndex] == input[i + matchLength]) {
            matchLength++;
          } else {
            break; // No longer matching
          }
        }
        
        if (matchLength > longestMatchLength) {
          longestMatchLength = matchLength;
          longestMatchOffset = i - startIndex;
        }
      }
      std::cout << "Longest match length: " << longestMatchLength << ", offset: " << longestMatchOffset << std::endl;
      // Pack the token and write it to output
      LZ77PackedToken packedToken = packToken(longestMatchOffset, longestMatchLength);
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
      std::cout << "No match found for: " << currentSubstring << std::endl;
      output[outputSize] = input[i];
      outputSize++;
      flagCounter++;
    }

    if (flagCounter == 8 || i == inputSize - 1) {
      // If we have 8 flags or reached the end of input, write the flag byte
      output[flagPos] = flagByte;
      flagPos = outputSize; // Update flag position for next byte
      outputSize++;
      flagByte = 0x00; // Reset the flag byte
      flagCounter = 0; // Reset the counter
    }
    std::cout << "Current output size: " << outputSize << std::endl;
    // Adding new substrings to the sliding window
    for (int j = 0; j < longestMatchLength; j++) {
      slidingWindowEnd++;
      std::cout << "Adding substring: " << std::string(input + slidingWindowEnd - MIN_MATCH + 1, MIN_MATCH) << std::endl;
      if (i + j + 1 < inputSize - MIN_MATCH) { // Ensure we don't go out of bounds
        std::string newSubstring(input + slidingWindowEnd - MIN_MATCH + 1, MIN_MATCH);
        windowSlide[newSubstring].push_back(slidingWindowEnd - MIN_MATCH + 1);
      }
    }

    while (slidingWindowEnd - slidingWindowStart + 1 > WINDOW_SIZE) {
      // If the sliding window exceeds the size, remove the oldest entry
      std::string oldestSubstring(input + slidingWindowStart, MIN_MATCH);
      if (windowSlide.find(oldestSubstring) != windowSlide.end()) {
        std::vector<int> &indices = windowSlide[oldestSubstring];
        // remove from the front
        indices.erase(indices.begin());
        if (indices.empty()) {
          windowSlide.erase(oldestSubstring); // Remove the entry if no indices left
        }
      }
      slidingWindowStart++;
    }

  }
  return outputSize;


}