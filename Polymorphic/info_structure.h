#pragma once

#include <stdint.h>

//consts (need to be moved)
#define POLYM_SERVICE_MAX_PEER_CONNECTIONS 100
#define POLYM_PEER_MAX_SERVICE_CONNECTIONS 20

// connection info specific to main service connections
typedef struct
{
	uint16_t serviceID; // the id # of the service this is running
	uint8_t serviceKey[4]; // key given to service on initialization
	char* serviceString; // the string that defines this service
} POLYM_SERVICE_INFO;

// connection info specific to auxiliary service connections
typedef struct
{
	uint16_t serviceID; // the id # of the service this is running
	uint16_t servicePort; // the service port # this aux service connection is on
} POLYM_SERVICE_AUX_INFO;

typedef struct
{
	uint16_t peerID; // we'll fill this out later
} POLYM_PEER_INFO;




typedef struct
{
	int connectionID; // services that are currently using this peer connection
	uint16_t serviceLocalPeerID; // the peer ID that this peer is on that service
} POLYM_CONNECTED_SERVICE_STATUS;

typedef struct
{
	POLYM_CONNECTED_SERVICE_STATUS connectedServices[POLYM_PEER_MAX_SERVICE_CONNECTIONS]; // services that are currently using this peer connection
	uint16_t connectedServicesCount; // number of currently connected services
} POLYM_PEER_STATUS;

typedef struct
{
	int connectedPeers[POLYM_SERVICE_MAX_PEER_CONNECTIONS]; // services that are currently using this peer connection
	uint16_t connectedPeersCount; // number of currently connected peers
} POLYM_SERVICE_STATUS;




typedef struct {
	uint8_t protocol; // the layer-4 protocol used for this connection
	uint8_t mode; // is this socket connected to a local service or a peer? POLYM_MODE_SERVICE or POLYM_MODE_PEER
	uint8_t addrtype; //IPv4 or v6

	union info // connection type specific information
	{
		POLYM_SERVICE_INFO service;
		POLYM_PEER_INFO peer;
		POLYM_SERVICE_AUX_INFO serviceAux;
	} mode_info;

	union status // connection type specific status
	{
		POLYM_PEER_STATUS peer;
		POLYM_SERVICE_STATUS service;
	} mode_status;

} POLYM_CONNECTION_INFO;