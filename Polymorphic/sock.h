#pragma once

#include <info_structure.h>

int initSocketLib();
int closeSocketLib();
int startListenSocket(char* port);

int sockRecv(void* connection, uint8_t *buffer, uint32_t length);
int sockSend(void* connection, uint8_t *buffer, uint32_t length);
int sockSendAsync(void* connection, uint8_t *buffer, uint32_t length);

void* openNewTCPConnection(char *ipAddress, char *l4Port, POLYM_CONNECTION_INFO **out_connectionInfo);

int32_t addNewService(void* connection);
int32_t removeService(uint16_t serviceID);
int32_t addNewServiceAux(uint16_t serviceID, void* connection);
int32_t removeServiceAux(uint16_t serviceID, uint16_t serviceAuxID);
int32_t addNewPeer(void* connection);
int32_t removePeer(uint16_t peerID);
void closeAllServiceAux(uint16_t serviceID);

int getCurrentServiceConnections(void** OUT_connectionArray, uint32_t maxCount);

void intIPtoStringIP(uint32_t ipv4AddressLong, char *outStringIP);

void* getConnectionFromPeerID(uint16_t peerID);
void* getConnectionFromServiceID(uint16_t serviceID, uint16_t portID);