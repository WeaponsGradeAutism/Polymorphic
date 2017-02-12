#pragma once
#include <stdint.h>
#include <info_structure.h>

void insertShortIntoBuffer(uint8_t *buffer, uint16_t unconvertedShort);
void insertShortIntoBufferNC(uint8_t *buffer, uint16_t unconvertedShort);
void insertLongIntoBuffer(uint8_t *buffer, uint32_t unconvertedLong);
void insertLongIntoBufferNC(uint8_t *buffer, uint32_t unconvertedLong);
uint16_t getShortFromBuffer(uint8_t* buffer);
uint16_t getShortFromBufferNC(uint8_t* buffer);
uint32_t getLongFromBuffer(uint8_t* buffer);
uint32_t getLongFromBufferNC(uint8_t* buffer);

const char* intIPtoStringIP(uint32_t ipv4AddressLong, char *OUT_StringIP, int bufferSize);

POLYM_MESSAGE_BUFFER* allocateBufferObject();