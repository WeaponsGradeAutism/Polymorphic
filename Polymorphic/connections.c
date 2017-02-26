#include <connections.h>
#include <definitions.h>
#include <stdlib.h>
#include <utils.h>
#include <sock.h>
#include <openssl\rand.h>

///<summary> Sends the polymorphic greeting over the supplied connection as a synchronous action. </summary>
///<returns> Error code on failure, 0 for success </returns>
int sendGreeting(void *connection)
{
	if (sizeof(POLYM_GREETING) != sockSend(connection, POLYM_GREETING, sizeof(POLYM_GREETING)))
		return POLY_REALM_FAILED;
	return 0;
}

///<summary> Send the supplied polymorphic error code over the supplied connection as a synchronous action. </summary>
void sendErrorCode(uint16_t errorCode, void *connection)
{
	uint8_t buffer[2];
	insertShortIntoBuffer(buffer, errorCode);
	sockSend(connection, buffer, 2);
}

///<summary> Shared code for initializing an info object for a new peer connection. </summary>
void initializeNewPeerInfo(POLYM_CONNECTION_INFO *connection_info, void *connection, uint16_t peerID)
{

	// initialize peer status struct
	int_array_init(&connection_info->realm_info.peer.status.connectedServices);
	int_array_init(&connection_info->realm_info.peer.status.outboundMessageQueue);

	// set realm to peer
	connection_info->realm = POLY_REALM_PEER;

	// add new peer to list
	connection_info->connectionID = peerID;
}

///<summary> Completely initializes a new POLY connection that has been recieved. Uses synchronous reads and sends. </summary>
///<param name='connection'> Connection socket for the incoming connection. </param>
///<param name='connection_info'> The connection info object associated with this connection. </param>
///<param name='out_connectionPointer'> (OUT) The connection object for the newly initialized connection. </param>
///<returns> Error code on failure, connection realm on success. <returns>
int initializeIncomingConnection(void *connection,  void** out_connectionPointer)
{

	// send greeting
	if (sendGreeting(connection) == POLY_REALM_FAILED) return POLY_REALM_FAILED;

	uint8_t buffer[8]; // create a re-usable buffer for operations

	// attempt to recieve encryption setting
	if (2 != sockRecv(connection, buffer, 2)) return POLY_REALM_FAILED;

	// handle encryption negotiation
	if (getShortFromBuffer(buffer) != 0) return POLY_REALM_FAILED; // TODO: encryption not yet implemented

	// attempt to recieve realm code
	if (2 != sockRecv(connection, buffer, 2)) return POLY_REALM_FAILED;

	POLYM_CONNECTION_INFO *connection_info;

	// connection realm
	switch (getShortFromBuffer(buffer))
	{

	case POLY_REALM_PEER:

	{
		uint16_t connectionID = allocateNewPeer(&connection_info);

		//construct the return buffer
		buffer[0] = 0;
		buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success

		if (2 != sockSend(connection, buffer, 2)) //send the success code and peer ID
		{
			freePeer(connectionID);
			return POLY_REALM_FAILED;
		}

		// initalize the peer connection
		initializeNewPeerInfo(connection_info, connection, connectionID);
		return POLY_REALM_PEER;
	}

	case POLY_REALM_SERVICE:
	{
		// this connection is a service
		// initialize new service connection
		uint16_t connectionID = allocateNewService(&connection_info);

		//attempt to recieve service string size
		if (2 != sockRecv(connection, buffer, 2))
		{
			freeService(connectionID);
			return POLY_REALM_FAILED;
		}

		// check validity of service string size, initialize string
		int serviceStringSize = getShortFromBuffer(buffer);
		if (serviceStringSize == 0)
		{
			freeService(connectionID);
			return POLY_REALM_FAILED;
		}
		connection_info->realm_info.service.serviceString = malloc(sizeof(uint8_t) * serviceStringSize);

		//get the service string
		if (serviceStringSize != sockRecv(connection, connection_info->realm_info.service.serviceString, serviceStringSize))
		{
			freeService(connectionID);
			return POLY_REALM_FAILED;
		}

		// TODO: require user approval

		// construct the last send buffer to send
		buffer[0] = 0;
		buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success

		if (2 != sockSend(connection, buffer, 8))
		{
			// if for some reason this fails, we need to remove the connection from the service list
			freeService(connectionID);
			return POLY_REALM_SERVICE; // do not return fail so that the connection isn't closed twice
		}
		connection_info->realm = POLY_REALM_SERVICE;
		connection_info->connectionID = connectionID;
		return POLY_REALM_SERVICE;
	}

	case POLY_REALM_CLIENT:

	{
		uint16_t connectionID = allocateNewClient(&connection_info);

		//construct the return buffer
		buffer[0] = 0;
		buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success

		if (2 != sockSend(connection, buffer, 2)) //send the success code and peer ID
		{
			freeClient(connectionID);
			return POLY_REALM_FAILED;
		}

		// initalize the client connection
		connection_info->realm = POLY_REALM_CLIENT;
		connection_info->connectionID = connectionID;
		return POLY_REALM_PEER;

	default:
		return POLY_REALM_FAILED;

	}
	}
}

///<summary> Handles the network negotiation of an outgoing connection. Uses synchronous reads and sends. </summary>
///<param name='ipAddress'> The IP address string to connect to. </param>
///<param name='l4Port'> The transport layer port to connection to. </param>
///<param name='protocol'> The protocol the connection will use. </param>
///<param name='out_connectionPointer'> (OUT) The connection object for the newly initialized connection. </param>
///<returns> Error code on failure, result code from remote server on success. <returns>
uint16_t initializeOutgoingConnection(char *ipAddress, uint16_t l4Port, uint8_t protocol, uint16_t *out_connectionID)
{
	POLYM_CONNECTION_INFO *connection_info;

	uint16_t connectionID = allocateNewPeer(&connection_info);

	char l4PortString[6];
	void *connection = openNewConnection(ipAddress, _itoa(l4Port, l4PortString, 10), &connection_info, protocol);

	if (connection == NULL)
		return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;

	uint8_t buffer[500];

	if (6 != sockRecv(connection, buffer, 6))
	{
		sendErrorCode(POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL, connection);
		removeConnection(connection_info);
		return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;
	}

	int index = 6;
	for (; index <= POLYM_GREETING_MAX_LENGTH; ++index)
	{

		if (1 != sockRecv(connection, &buffer[index], 1))
		{
			sendErrorCode(POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL, connection);
			removeConnection(connection_info);
			return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;
		}

		if (buffer[index] == '\n')
		{
			break;
		}
	}

	if (index > POLYM_GREETING_MAX_LENGTH) // greeting is too long, close the connection
		closeConnectionSocket(connection);

	insertShortIntoBuffer(buffer, 0); // encryption mode
	insertShortIntoBuffer(&buffer[2], 1); // realm code

	sockSend(connection, buffer, 4); // send the connection request

	if (2 != sockRecv(connection, buffer, 2))
	{
		sendErrorCode(POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL, connection);
		removeConnection(connection_info);
		return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;
	}

	initializeNewPeerInfo(connection_info, connection, connectionID);

	// TODO: send service update request

	*out_connectionID = connectionID;
	return getShortFromBuffer(buffer); //return result code recieved from remote server

}

///<summary> Processes a request from a service to connect to the given peer. </summary>
void connectServiceToPeer(POLYM_CONNECTION_INFO *service, uint32_t address, uint16_t port, uint8_t protocol)
{

	// check to see if the peer is already connected
	int peerID = intAddressConnected(address, port, protocol);

	// peer is already connected to this client
	if (peerID != -1) 
	{
		void *connectionPointer = getConnectionFromPeerID(peerID);
		POLYM_CONNECTION_INFO *peer = getInfoFromConnection(connectionPointer);
		
		// check to see if the peer is already connected to this service
		if (int_array_find(&service->realm_info.service.status.connectedPeers, peerID) > -1)
		{
			// TODO: send PEER_CONNECTION command to service to remind it of the existing peer id it is already associated with
		}

		// if not, add it to the service's list of connected peers and vice versa
		int_vector_append(&service->realm_info.service.status.connectedPeers, peerID);
		lockConnectionMutexByInfo(peer);
		int_vector_append(&peer->realm_info.service.status.connectedPeers, service->connectionID);
		unlockConnectionMutexByInfo(peer);
		// TODO: send PEER_CONNECTION command to service to inform it that it is now associated with the peer id

	}
	else
	{
		// TODO: initiate new connection
	}

}

///<summary> Remove the supplied service from the connection list and close it. </summary>
void removeConnection(POLYM_CONNECTION_INFO *connection_info)
{
	switch (connection_info->realm)
	{

	case POLY_REALM_SERVICE:
		// close a service control connection
		removeService(connection_info->connectionID);
		break;

	case POLY_REALM_PEER:
		// close a peer connection
		removePeer(connection_info->connectionID);
		break;

	default:
		break;
	}
}