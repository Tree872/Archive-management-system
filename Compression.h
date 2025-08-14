#pragma once
#include <stdint.h>
#define WINDOW_SIZE 1024
#define LOOKAHEAD_SIZE 10

typedef struct {
  uint16_t distance;
  uint8_t length;
} LZ77Token;

size_t lz77_compress(const uint8_t* input, size_t inputSize, uint8_t* output);