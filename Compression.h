#pragma once
#include <stdint.h>
#define WINDOW_SIZE 4096

typedef uint16_t LZ77PackedToken;

int lzssEncode(const char* input, int inputSize, char* output);
void unpackToken(uint16_t packed, uint16_t* offset, uint8_t* length);
LZ77PackedToken packToken(uint16_t offset, uint8_t length);
