#pragma once

#include <info_structure.h>

int closeListenSocket(); 
int startListenSocket(char* port); 

int sockRecv(void* connection, uint8_t *buffer, uint32_t length);
int sockSend(void* connection, uint8_t *buffer, uint32_t length);
int sockSendAsync(void* connection, POLYM_MESSAGE_BUFFER *buffer);

void* openNewConnection(char *ipAddress, char *l4Port, POLYM_CONNECTION_INFO **out_connectionInfo, uint8_t protocol);

void lockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info);
void unlockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info);

uint16_t allocateNewService(void **out_connectionInfoPointer);
int addNewService(POLYM_CONNECTION_INFO *connection);
void* freeService(uint16_t serviceID);
int removeService(uint16_t serviceID);
uint16_t allocateNewPeer(void **out_connectionInfoPointer);
void* freePeer(uint16_t peerID);
int removePeer(uint16_t peerID);
uint16_t allocateNewClient(void **out_connectionInfoPointer);
int removeClient(uint16_t clientID);
void* freeClient(uint16_t clientID);
int closeConnectionSocket(void *connection);

void* getConnectionFromPeerID(uint16_t peerID);
void* getConnectionFromServiceID(uint16_t serviceID);
void* getConnectionFromClientID(uint16_t clientID);
POLYM_CONNECTION_INFO* getInfoFromConnection(void *connection);
//POLYM_CONNECTION_INFO* getServiceConnectionInfo(int index); deprecated (LUL)
int getCurrentServiceConnections(void** OUT_connectionArray, unsigned int maxCount);
int getCurrentPeerConnections(void** OUT_connectionArray, unsigned int maxCount);
int getCurrentClientConnections(void** OUT_connectionArray, unsigned int maxCount);

int addressConnected(char *stringAddress, uint16_t port, uint8_t protocol);