#pragma once

#include <info_structure.h>

int initSocketLib();
int closeSocketLib();
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
int sockSendAsync(void* connection, uint8_t *buffer, int length);

void* openNewConnection(char *ipAddress, char *l4Port, POLYM_CONNECTION_INFO **out_connectionInfo, uint8_t protocol);

int addNewService(void* connection, void** out_connectionPointer);
int32_t removeService(uint16_t serviceID);
int addNewServiceAux(uint16_t serviceID, void* connection, void** out_connectionPointer);
int32_t removeServiceAux(uint16_t serviceID, uint16_t serviceAuxID);
int addNewPeer(void* connection, void **out_connectionPointer);
int32_t removePeer(uint16_t peerID);
void closeAllServiceAux(uint16_t serviceID);
int closeUnitializedConnection(void *connection);

const char* intIPtoStringIP(uint32_t ipv4AddressLong, char *OUT_StringIP, int bufferSize);

void* getConnectionFromPeerID(uint16_t peerID);
void* getConnectionFromServiceID(uint16_t serviceID, uint16_t portID);
POLYM_CONNECTION_INFO* getInfoFromConnection(void *connection);
POLYM_CONNECTION_INFO* getServiceConnectionInfo(int index);
int getCurrentServiceConnections(void** OUT_connectionArray, uint32_t maxCount); // returns a full list of all the current connected services, up to maxCount
int getCurrentPeerConnections(void** OUT_connectionArray, uint32_t maxCount); // returns a full list of all the current connected peers, up to maxCount

int addressConnected(char *stringAddress, uint16_t port, uint8_t protocol);

void lockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info);
void unlockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info);