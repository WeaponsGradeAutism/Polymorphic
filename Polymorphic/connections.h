#pragma once

#include <stdint.h>
#include <sock.h>

#define POLYM_GREETING "POLY v0.1, Polymorphic client v0.1 at your misfortune.\n"
#define POLYM_GREETING_MAX_LENGTH 500

int initializeIncomingConnection(void *connection, POLYM_CONNECTION_INFO *connection_info, void** out_connectionPointer);
uint16_t initializeOutgoingConnection(char *ipAddress, uint16_t l4Port, uint8_t protocol, void **out_connectionPointer);
void removeConnection(POLYM_CONNECTION_INFO *connection_info);

