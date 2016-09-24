#pragma once

#include <stdint.h>
#include <sock.h>

#define POLYMORPHIC_GREETING "POLY v0.1, Polymorphic client v0.1 at your misfortune.\r\n"

#define POLYM_ARRAY_PUSH_SUCCESS 0
#define POLYM_ARRAY_PUSH_SUCCESS_GROW 1
#define POLYM_ARRAY_PUSH_ERROR_FULL 2

int initializeIncomingConnection(void *connection, POLYM_CONNECTION_INFO *connection_info, void** out_connectionPointer);
uint16_t initializeOutgoingConnection(char *ipAddress, uint16_t l4Port, uint8_t protocol, void **out_connectionPointer);
void cleanupConnection(POLYM_CONNECTION_INFO *connection_info);

