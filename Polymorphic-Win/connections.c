#include <connections.h>
#include <definitions.h>
#include <stdlib.h>
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
void initializeNewPeerInfo(POLYM_CONNECTION_INFO *connection_info, void *connection, void **out_connectionPointer)
{

	// initialize peer status struct
	int_array_init(&connection_info->realm_info.peer.status.connectedServices);
	int_array_init(&connection_info->realm_info.peer.status.outboundMessageQueue);

	// set realm to peer
	connection_info->realm = POLY_REALM_PEER;

	// add new peer to list
	connection_info->realm_info.peer.peerID = addNewPeer(connection, out_connectionPointer);
}

///<summary> Completely initializes a new POLY connection that has been recieved. Uses synchronous reads and sends. </summary>
///<param name='connection'> Connection socket for the incoming connection. </param>
///<param name='connection_info'> The connection info object associated with this connection. </param>
///<param name='out_connectionPointer'> (OUT) The connection object for the newly initialized connection. </param>
///<returns> Error code on failure, connection realm on success. <returns>
int initializeIncomingConnection(void *connection, POLYM_CONNECTION_INFO *connection_info, void** out_connectionPointer)
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

	// connection realm
	switch (getShortFromBuffer(buffer))
	{

	case POLY_REALM_PEER:

		//construct the return buffer
		buffer[0] = 0;
		buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success

		if (2 == sockSend(connection, buffer, 2)) //send the success code and peer ID
		{
			// initalize the peer connection
			initializeNewPeerInfo(connection_info, connection, out_connectionPointer);
		} 
		else
			return POLY_REALM_FAILED;

		return POLY_REALM_PEER;

	case POLY_REALM_SERVICE:
		// this connection is a service

		// initialize new service connection
		connection_info->realm = POLY_REALM_SERVICE;

		//attempt to recieve service string size
		if (2 != sockRecv(connection, buffer, 2))
		{
			return POLY_REALM_FAILED;
		}

		// check validity of service string size, initialize string
		int serviceStringSize = getShortFromBuffer(buffer);
		if (serviceStringSize < 1)
		{
			return POLY_REALM_FAILED;
		}
		connection_info->realm_info.service.serviceString = malloc(sizeof(uint8_t) * serviceStringSize);

		//get the service string
		if (serviceStringSize != sockRecv(connection, connection_info->realm_info.service.serviceString, serviceStringSize))
		{
			return POLY_REALM_FAILED;
		}

		// attempt to add a new service to the internal list of service connections
		// TODO: require user approval
		connection_info->realm_info.service.serviceID = addNewService(connection, out_connectionPointer);

		// if adding new service fails, send error code and abort.
		// TODO: better error handling
		if (connection_info->realm_info.service.serviceID == -1)
		{
			return POLY_REALM_FAILED;
		}

		// construct the last send buffer to send
		buffer[0] = 0;
		buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success

		if (2 != sockSend(connection, buffer, 8))
		{
			// if for some reason this fails, we need to remove the connection from the service list
			removeService(connection_info->realm_info.service.serviceID);
			return POLY_REALM_SERVICE; // do not return fail so that the connection isn't closed twice
		}
		else
			return POLY_REALM_SERVICE;

	case POLY_REALM_CLIENT:

		//construct the return buffer
		buffer[0] = 0;
		buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success

		if (2 == sockSend(connection, buffer, 2)) //send the success code and peer ID
		{
			// initalize the client connection
			connection_info->realm = POLY_REALM_CLIENT;
			connection_info->realm_info.client.clientID = addNewClient(connection, out_connectionPointer);
		}
		else
			return POLY_REALM_FAILED;

		return POLY_REALM_PEER;

	default:
		return POLY_REALM_FAILED;

	}
}

///<summary> Completely initializes a new outbound POLY connection to a peer. Uses synchronous reads and sends. </summary>
///<param name='ipAddress'> The IP address string to connect to. </param>
///<param name='l4Port'> The transport layer port to connection to. </param>
///<param name='protocol'> The protocol the connection will use. </param>
///<param name='out_connectionPointer'> (OUT) The connection object for the newly initialized connection. </param>
///<returns> Error code on failure, result code from remote server on success. <returns>
uint16_t initializeOutgoingConnection(char *ipAddress, uint16_t l4Port, uint8_t protocol, void **out_connectionPointer)
{
	POLYM_CONNECTION_INFO *info;
	char l4PortString[6];
	void *connection = openNewConnection(ipAddress, _itoa(l4Port, l4PortString, 10), &info, protocol);

	if (connection == NULL)
		return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;

	uint8_t buffer[500];

	if (6 != sockRecv(connection, buffer, 6))
	{
		sendErrorCode(POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL, connection);
		removeConnection(info);
		return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;
	}

	int index = 6;
	for (; index <= POLYM_GREETING_MAX_LENGTH; ++index)
	{

		if (1 != sockRecv(connection, &buffer[index], 1))
		{
			sendErrorCode(POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL, connection);
			removeConnection(info);
			return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;
		}

		if (buffer[index] == '\n')
		{
			break;
		}
	}

	if (index > POLYM_GREETING_MAX_LENGTH) // greeting is too long, close the connection
		closeUnitializedConnection(connection);

	insertShortIntoBuffer(buffer, 0); // encryption mode
	insertShortIntoBuffer(&buffer[2], 1); // realm code

	sockSend(connection, buffer, 4); // send the connection request

	if (2 != sockRecv(connection, buffer, 2))
	{
		sendErrorCode(POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL, connection);
		removeConnection(info);
		return POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL;
	}

	initializeNewPeerInfo(info, connection, out_connectionPointer);

	// TODO: send service update request

	return getShortFromBuffer(buffer); //return result code recieved from remote server

}

///<summary> Remove the supplied service from the connection list and close it. </summary>
void removeConnection(POLYM_CONNECTION_INFO *connection_info)
{
	switch (connection_info->realm)
	{

	case POLY_REALM_SERVICE:
		// close a service control connection
		removeService(connection_info->realm_info.service.serviceID);
		break;

	case POLY_REALM_PEER:
		// close a peer connection
		removePeer(connection_info->realm_info.peer.peerID);
		break;

	default:
		break;
	}
}