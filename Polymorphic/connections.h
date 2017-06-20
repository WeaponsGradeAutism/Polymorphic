#pragma once

#include <stdint.h>
#include <sock.h>

#define POLYM_GREETING "POLY v0.1, Polymorphic v0.1, at your misfortune.\n"
#define POLYM_GREETING_MAX_LENGTH 500

int initializeIncomingConnection(void *connection, void** out_connectionPointer);
uint16_t initializeOutgoingConnection(char *ipAddress, uint16_t l4Port, uint8_t protocol, POLYM_CONNECTION_INFO **out_connectionInfo);
void removeConnection(POLYM_CONNECTION_INFO *connection_info);

void connectServiceToPeer(POLYM_CONNECTION_INFO *service, uint32_t address, uint16_t port, uint8_t protocol);
