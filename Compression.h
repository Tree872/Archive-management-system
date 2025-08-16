#pragma once
#include <stdint.h>

typedef uint16_t LZSSPackedToken;

int lzssEncode(const char* input, int inputSize, char* output);
int lzssDecode(const char* input, int inputSize, char* output);
void unpackToken(LZSSPackedToken packed, uint16_t* offset, uint8_t* length);

LZSSPackedToken packToken(uint16_t offset, uint8_t length);
