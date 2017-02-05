#pragma once

#include <stdint.h>
#include <vector.h>

//consts (need to be moved)
#define POLYM_SERVICE_MAX_PEER_CONNECTIONS 100
#define POLYM_PEER_MAX_SERVICE_CONNECTIONS 20

// mode specific connection state; initialized in connections.c when connections are initialized
// Data is MUTABLE.
typedef struct
{
	int_array connectedServices; // services that are currently using this peer connection
	int_array outboundMessageQueue; // a list of services that have messages queued to be sent to this peer
} POLYM_PEER_STATUS;

typedef struct
{
	int_array connectedPeers; // services that are currently using this peer connection
} POLYM_SERVICE_STATUS;

// mode specific connection info; initialized in connections.c when connections are initialized
typedef struct
{
	POLYM_SERVICE_STATUS status;
	uint16_t serviceID; // the id # of the service this is running
	uint8_t serviceKey[4]; // key given to service on initialization
	char* serviceString; // the string that defines this service
} POLYM_SERVICE_INFO;

typedef struct
{
	POLYM_PEER_STATUS status;
	uint16_t peerID; // we'll fill this out later
} POLYM_PEER_INFO;

typedef struct
{
	uint16_t clientID; // we'll fill this out later
} POLYM_CLIENT_INFO;


// Connection info structure. Contains conneciton info shared between the socket implementation and the commands implementation.
// This data is also immutable and can be considered thread-safe, EXCEPT FOR THE STATUS UNION.
// Status data is mutable, and so reads and writes must be synchronized using the lock/unlockConnectionMutexByInfo interfaces.

typedef struct {
	
	uint8_t mode; // is this socket connected to a local service or a peer? POLY_REALM_SERVICE or POLY_REALM_PEER
	uint16_t connectionID; // the ID of the given connection on its mode

	union info // connection type specific information
	{
		POLYM_SERVICE_INFO service;
		POLYM_PEER_INFO peer;
		POLYM_CLIENT_INFO client;
	} mode_info;

} POLYM_CONNECTION_INFO;