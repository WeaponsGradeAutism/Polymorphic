#pragma once

#include <stdint.h>
#include <vector.h>
#include <definitions.h>

//consts (need to be moved)
#define POLYM_SERVICE_MAX_PEER_CONNECTIONS 100
#define POLYM_PEER_MAX_SERVICE_CONNECTIONS 20

typedef struct
{
	char buf[POLY_MAX_SERVICE_STRING_SIZE]; // Buffer size range: POLY_MAX_SERVICE_STRING_SIZE
	int stringSize;
	void* containerObject; // used internally to track the message_buffer object that contains this one
} POLYM_SERVICE_STRING;

// realm specific connection state; initialized in connections.c when connections are initialized
// Data is MUTABLE.
typedef struct
{
	int_array connectedServices; // services that are currently using this peer connection
	//TODO: implement tracking, prune connection when no services are connected for some time
	int_array outboundMessageQueue; // a list of services that have messages queued to be sent to this peer
} POLYM_PEER_STATUS;

typedef struct
{
	int_array connectedPeers; // services that are currently using this status connection
} POLYM_SERVICE_STATUS;

typedef struct
{
	uint16_t dummy; // placeholder
} POLYM_CLIENT_STATUS;

// realm specific connection info; initialized in connections.c when connections are initialized
typedef struct
{
	POLYM_SERVICE_STATUS status;
	POLYM_SERVICE_STRING* serviceString; // the string that defines this service
} POLYM_SERVICE_INFO;

typedef struct
{
	POLYM_PEER_STATUS status;
} POLYM_PEER_INFO;

typedef struct
{
	POLYM_CLIENT_STATUS dummy;
} POLYM_CLIENT_INFO;


// Connection info structure. Contains conneciton info shared between the socket implementation and the commands implementation.
// This data is also immutable and can be considered thread-safe, EXCEPT FOR THE STATUS UNION.
// Status data is mutable, and so reads and writes must be synchronized using the lock/unlockConnectionMutexByInfo interfaces.

typedef struct {
	
	uint8_t realm; // is this socket connected to a local service or a peer? POLY_REALM_SERVICE or POLY_REALM_PEER
	uint16_t connectionID; // the ID of the given connection on its realm

	union info // connection type specific information
	{
		POLYM_SERVICE_INFO service;
		POLYM_PEER_INFO peer;
		POLYM_CLIENT_INFO client;
	} realm_info;

} POLYM_CONNECTION_INFO;

typedef struct
{
	uint8_t buf[POLY_MAX_MESSAGE_SIZE];
	int messageSize;
	void* containerObject; // used internally to track the message_buffer object that contains this one
} POLYM_MESSAGE_BUFFER;