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

int addNewService(void* connection, void** out_connectionPointer);
int removeService(uint16_t serviceID);
int addNewPeer(void* connection, void **out_connectionPointer);
int removePeer(uint16_t peerID);
int addNewClient(void* connection, void **out_connectionPointer);
int removeClient(uint16_t clientID);
int closeUnitializedConnection(void *connection);

void* getConnectionFromPeerID(uint16_t peerID);
void* getConnectionFromServiceID(uint16_t serviceID);
void* getConnectionFromClientID(uint16_t clientID);
POLYM_CONNECTION_INFO* getInfoFromConnection(void *connection);
//POLYM_CONNECTION_INFO* getServiceConnectionInfo(int index); deprecated (LUL)
int getCurrentServiceConnections(void** OUT_connectionArray, unsigned int maxCount);
int getCurrentPeerConnections(void** OUT_connectionArray, unsigned int maxCount);
int getCurrentClientConnections(void** OUT_connectionArray, unsigned int maxCount);

int addressConnected(char *stringAddress, uint16_t port, uint8_t protocol);