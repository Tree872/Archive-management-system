#include <stdint.h>
#include "Compression.h"

size_t lz77_compress(const uint8_t* input, size_t inputSize, uint8_t* output) {
  size_t inPos = 0, outPos = 0;
  uint8_t flagByte = 0;
  int flagCount = 0;
  size_t flagPos = 0;

  while (inPos < inputSize) {
    // Reserve space for flag byte every 8 items
    if (flagCount == 0) {
      flagPos = outPos;
      output[outPos++] = 0; // placeholder
    }

    // Search for the longest match in sliding window
    size_t bestLen = 0;
    size_t bestDist = 0;
    size_t windowStart = (inPos > WINDOW_SIZE) ? inPos - WINDOW_SIZE : 0;

    for (size_t j = windowStart; j < inPos; j++) {
      size_t matchLen = 0;
      while (matchLen < LOOKAHEAD_SIZE &&
        inPos + matchLen < inputSize &&
        input[j + matchLen] == input[inPos + matchLen]) {
        matchLen++;
      }
      if (matchLen > bestLen && matchLen >= 3) {
        bestLen = matchLen;
        bestDist = inPos - j;
      }
    }

    if (bestLen >= 3) {
      // Token: distance(2 bytes) + length(1 byte)
      flagByte |= (1 << flagCount);
      output[outPos++] = (bestDist >> 8) & 0xFF;
      output[outPos++] = bestDist & 0xFF;
      output[outPos++] = bestLen;
      inPos += bestLen;
    }
    else {
      // Literal
      output[outPos++] = input[inPos++];
    }

    flagCount++;

    // Flush flag byte if full
    if (flagCount == 8) {
      output[flagPos] = flagByte;
      flagCount = 0;
      flagByte = 0;
    }

    // Early stop if compressed already larger
    if (outPos >= inputSize) {
      return 0; // signal caller to store uncompressed
    }
  }

  // Flush remaining flag byte
  if (flagCount > 0) {
    output[flagPos] = flagByte;
  }

  return outPos;
}