#pragma once
#include <stdint.h>

#define MIN_MATCH 3
#define MAX_MATCH 18
#define WINDOW_SIZE 4096
#define MAX_OCCURRENCE_PER_SUBSTRING 64

typedef struct {
  int buffer[MAX_OCCURRENCE_PER_SUBSTRING];
  int head;
  int tail;
  int length;
} IndexQueue;

typedef uint16_t LZSSPackedToken;

int lzssEncode(const char* input, int inputSize, char* output);
int lzssDecode(const char* input, int inputSize, char* output);
void unpackToken(LZSSPackedToken packed, uint16_t* offset, uint8_t* length);

LZSSPackedToken packToken(uint16_t offset, uint8_t length);
uint32_t packToUInt32(const char* data);

int popIndexQueue(IndexQueue* queue);
void pushIndexQueue(IndexQueue* queue, int index);
void initIndexQueue(IndexQueue* queue);