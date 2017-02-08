#pragma once

#include <info_structure.h>

int closeListenSocket(); 
int startListenSocket(char* port); 

void insertShortIntoBuffer(uint8_t *buffer, uint16_t unconvertedShort); 
void insertShortIntoBufferNC(uint8_t *buffer, uint16_t unconvertedShort);
void insertLongIntoBuffer(uint8_t *buffer, uint32_t unconvertedLong);
void insertLongIntoBufferNC(uint8_t *buffer, uint32_t unconvertedLong);
uint16_t getShortFromBuffer(uint8_t* buffer);
uint16_t getShortFromBufferNC(uint8_t* buffer);
uint32_t getLongFromBuffer(uint8_t* buffer);
uint32_t getLongFromBufferNC(uint8_t* buffer);

int sockRecv(void* connection, uint8_t *buffer, uint32_t length);
int sockSend(void* connection, uint8_t *buffer, uint32_t length);
int sockSendAsync(void* connection, POLYM_MESSAGE_BUFFER *buffer);

void* openNewConnection(char *ipAddress, char *l4Port, POLYM_CONNECTION_INFO **out_connectionInfo, uint8_t protocol);

int addNewService(void* connection, void** out_connectionPointer);
int removeService(uint16_t serviceID);
int addNewPeer(void* connection, void **out_connectionPointer);
int removePeer(uint16_t peerID);
int addNewClient(void* connection, void **out_connectionPointer);
int removeClient(uint16_t clientID);
int closeUnitializedConnection(void *connection);

const char* intIPtoStringIP(uint32_t ipv4AddressLong, char *OUT_StringIP, int bufferSize);

void* getConnectionFromPeerID(uint16_t peerID);
void* getConnectionFromServiceID(uint16_t serviceID);
void* getConnectionFromClientID(uint16_t clientID);
POLYM_CONNECTION_INFO* getInfoFromConnection(void *connection);
//POLYM_CONNECTION_INFO* getServiceConnectionInfo(int index); deprecated (LUL)
int getCurrentServiceConnections(void** OUT_connectionArray, unsigned int maxCount);
int getCurrentPeerConnections(void** OUT_connectionArray, unsigned int maxCount);
int getCurrentClientConnections(void** OUT_connectionArray, unsigned int maxCount);

int addressConnected(char *stringAddress, uint16_t port, uint8_t protocol);

void lockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info);
void unlockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info);

POLYM_MESSAGE_BUFFER* allocateBufferObject();