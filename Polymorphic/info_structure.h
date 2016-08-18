#pragma once

#include <stdint.h>

//consts (need to be moved)
#define SERVICE_MAX_PEER_CONNECTIONS 100
#define PEER_MAX_SERVICE_CONNECTIONS 20




// connection info specific to main service connections
typedef struct
{
	uint16_t serviceID; // the id # of the service this is running
	uint8_t serviceKey[4]; // key given to service on initialization
	char* serviceString; // the string that defines this service
} SERVICE_INFO;

// connection info specific to auxiliary service connections
typedef struct
{
	uint16_t serviceID; // the id # of the service this is running
	uint16_t servicePort; // the service port # this aux service connection is on
} SERVICE_AUX_INFO;

typedef struct
{
	uint16_t peerID; // we'll fill this out later
} PEER_INFO;




typedef struct
{
	int connectionID; // services that are currently using this peer connection
	uint16_t serviceLocalPeerID; // the peer ID that this peer is on that service
} CONNECTED_SERVICE_STATUS;

typedef struct
{
	CONNECTED_SERVICE_STATUS connectedServices[PEER_MAX_SERVICE_CONNECTIONS]; // services that are currently using this peer connection
	uint16_t connectedServicesIndex; // current position on connectedServices array
	uint16_t connectedServicesCount; // number of currently connected services
} PEER_STATUS;

typedef struct
{
	int connectedPeers[SERVICE_MAX_PEER_CONNECTIONS]; // services that are currently using this peer connection
	uint16_t connectedPeersIndex; // current position on connectedPeers array
	uint16_t connectedPeersCount; // number of currently connected peers
} SERVICE_STATUS;




typedef struct {
	uint8_t protocol; // the layer-4 protocol used for this connection
	uint8_t mode; // is this socket connected to a local service or a peer? POLY_MODE_SERVICE or POLY_MODE_PEER
	uint8_t addrtype; //IPv4 or v6

	union info // connection type specific information
	{
		SERVICE_INFO service;
		PEER_INFO peer;
		SERVICE_AUX_INFO serviceAux;
	} mode_info;

	union status // connection type specific status
	{
		PEER_STATUS peer;
		SERVICE_STATUS service;
	} mode_status;

} CONNECTION_INFO;