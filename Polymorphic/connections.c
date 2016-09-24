#include <connections.h>
#include <definitions.h>
#include <stdlib.h>
#include <openssl\rand.h>

int sendGreeting(void *connection)
{
	if (sizeof(POLYMORPHIC_GREETING) != sockSend(connection, POLYMORPHIC_GREETING, sizeof(POLYMORPHIC_GREETING)))
		return POLYM_MODE_FAILED;
	return 0;
}

void sendErrorCode(uint16_t errorCode, void *connection)
{
	uint8_t buffer[2];
	insertShortIntoBuffer(buffer, errorCode);
	sockSend(connection, buffer, 2);
}

void initializeNewPeerInfo(POLYM_CONNECTION_INFO *connection_info, void *connection, void **out_connectionPointer)
{

	// initialize peer status struct
	int_array_init(&connection_info->mode_status.peer.connectedServices);
	int_array_init(&connection_info->mode_status.peer.outboundMessageQueue);

	// set mode to peer
	connection_info->mode = POLYM_MODE_PEER;

	//initialize the int array containing the current connected services
	int_array_init(&connection_info->mode_status.peer.connectedServices);

	// add new peer to list
	connection_info->mode_info.peer.peerID = addNewPeer(connection, out_connectionPointer);
}

int initializeIncomingConnection(void *connection, POLYM_CONNECTION_INFO *connection_info, void** out_connectionPointer)
{

	// send greeting
	if (sendGreeting(connection) == POLYM_MODE_FAILED)
	{
		return POLYM_MODE_FAILED;
	}

	uint8_t buffer[8]; // create a buffer for operations

					   // attempt to recieve encryption setting
	if (2 != sockRecv(connection, buffer, 2))
		return POLYM_MODE_FAILED;

	// handle encryption negotiation
	if (getShortFromBuffer(buffer) != 0)
	{
		return POLYM_MODE_FAILED; // encryption not yet implemented
	}

	// attempt to recieve realm code
	if (2 != sockRecv(connection, buffer, 2))
	{
		sendErrorCode(POLYM_ERROR_REALM_CODE_FAIL, connection);
		return POLYM_MODE_FAILED;
	}

	// 0 for service, anything else for peer.
	if (0 == getShortFromBuffer(buffer))
	{
		// this connection is a service

		// initalize connected peer array
		int_array_init(&connection_info->mode_status.service.connectedPeers);

		// attempt to recieve new or existing service setting
		if (2 != sockRecv(connection, buffer, 2))
		{
			sendErrorCode(POLYM_ERROR_NEWEXISTING_CODE_FAIL, connection);
			return POLYM_MODE_FAILED;
		}

		uint16_t serviceID = getShortFromBuffer(buffer);

		// 0 for new, all else for existing service ID.
		if (serviceID == 0)
		{
			// initialize new service connection
			connection_info->mode = POLYM_MODE_SERVICE;

			//attempt to recieve service string size
			if (2 != sockRecv(connection, buffer, 2))
			{
				sendErrorCode(POLYM_ERROR_SERVICE_STRING_SIZE_FAIL, connection);
				return POLYM_MODE_FAILED;
			}

			// check validity of service string size, initialize string
			int serviceStringSize = getShortFromBuffer(buffer);
			if (serviceStringSize < 1)
			{
				sendErrorCode(POLYM_ERROR_INVALID_SERVICE_STRING_SIZE, connection);
				return POLYM_MODE_FAILED;
			}
			connection_info->mode_info.service.serviceString = malloc(sizeof(uint8_t) * serviceStringSize);

			// get 4 random bytes for service key
			RAND_bytes(connection_info->mode_info.service.serviceKey, 4);

			//get the service string
			if (serviceStringSize != sockRecv(connection, connection_info->mode_info.service.serviceString, serviceStringSize))
			{
				sendErrorCode(POLYM_ERROR_SERVICE_STRING_FAIL, connection_info);
				return POLYM_MODE_FAILED;
			}

			// attempt to add a new service to the internal list of service connections
			// TODO: require user approval
			connection_info->mode_info.service.serviceID = addNewService(connection, out_connectionPointer);

			// if adding new service fails, send error code and abort.
			// TODO: better error handling
			if (connection_info->mode_info.service.serviceID == -1)
			{
				sendErrorCode(POLYM_ERROR_SERVICE_CREATION_FAIL, connection);
				return POLYM_MODE_FAILED;
			}

			// construct the last send buffer to send
			buffer[0] = 0;
			buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success
			insertShortIntoBuffer(&buffer[2], (uint16_t)connection_info->mode_info.service.serviceID); // insert the service ID short into the buffer with octets 2 and 3
			buffer[4] = connection_info->mode_info.service.serviceKey[0];
			buffer[5] = connection_info->mode_info.service.serviceKey[1];
			buffer[6] = connection_info->mode_info.service.serviceKey[2];
			buffer[7] = connection_info->mode_info.service.serviceKey[3]; // set the last 4 bytes of the buffer to the service key.

																		  // send the buffer
			if (8 != sockSend(connection, buffer, 8))
			{
				// if for some reason this fails, we need to remove the connection from the service listS
				removeService(connection_info->mode_info.service.serviceID);
				return POLYM_MODE_SERVICE; // do not return fail so that the connection isn't closed twice
			}
			else
				return POLYM_MODE_SERVICE;
		}
		else
		{

			// initalize connected peer array
			int_array_init(&connection_info->mode_status.service.connectedPeers);

			// initialize existing service connection
			connection_info->mode = POLYM_MODE_SERVICE_AUX;

			// get the info structure for this service
			POLYM_CONNECTION_INFO* serviceInfo = getServiceConnectionInfo(serviceID);

			// if the acquisition fails, close the connection
			if (serviceInfo == NULL)
			{
				sendErrorCode(POLYM_ERROR_SERVICE_DOES_NOT_EXIST, connection);
				return POLYM_MODE_FAILED;
			}

			// recieve the service key
			if (4 != sockRecv(connection, buffer, 4))
			{
				sendErrorCode(POLYM_ERROR_SERVICE_KEY_FAIL, connection);
				return POLYM_MODE_FAILED;
			}

			// validate the service key
			if (0 != strncmp(buffer, serviceInfo->mode_info.service.serviceKey, 4))
			{
				sendErrorCode(POLYM_ERROR_SERVICE_KEY_INVALID, connection);
				return POLYM_MODE_FAILED;
			}

			// set the service ID and port info
			connection_info->mode_info.serviceAux.serviceID = serviceID;
			connection_info->mode_info.serviceAux.servicePort = addNewServiceAux(serviceID, connection, out_connectionPointer);

			// construct the last send buffer to send
			buffer[0] = 0;
			buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success
			insertShortIntoBuffer(&buffer[2], (uint16_t)connection_info->mode_info.serviceAux.servicePort); // set 3rd and 4th bytes to the service port

																										   // send result code and port number
			if (4 != sockSend(connection, buffer, 4))
			{
				// if for some reason this fails, we'll need to dispose of the connection
				removeServiceAux(serviceID, connection_info->mode_info.serviceAux.servicePort);
				return POLYM_MODE_SERVICE_AUX; // do not return fail so that the connection isn't closed twice
			}
			else
				return POLYM_MODE_SERVICE_AUX;
		}
	}
	else
	{

		// initalize the peer connection
		initializeNewPeerInfo(connection_info, connection, out_connectionPointer);

		//construct the return buffer
		buffer[0] = 0;
		buffer[1] = 0; // set the first two bytes (result code) to 0 to indicate success

		if (2 != sockSend(connection, buffer, 2)) //send the success code and peer ID
		{
			// if for some reason this fails, we'll need to dispose of the connection
			removePeer(connection_info->mode_info.peer.peerID);
			return POLYM_MODE_FAILED;
		}

		return POLYM_MODE_PEER;
	}
}

uint16_t initializeOutgoingConnection(char *ipAddress, uint16_t l4Port, uint8_t protocol, void **out_connectionPointer)
{
	POLYM_CONNECTION_INFO *info;
	char l4PortString[6];
	void *connection = openNewConnection(ipAddress, _itoa(l4Port, l4PortString, 10), &info, protocol);

	if (connection == NULL)
		return POLYM_ERROR_CONNECTION_FAIL;

	uint8_t buffer[500];

	if (6 != sockRecv(connection, buffer, 6))
	{
		sendErrorCode(POLYM_ERROR_CONNECTION_FAIL, connection);
		cleanupConnection(info);
		return POLYM_ERROR_CONNECTION_FAIL;
	}

	uint8_t newlineTerminus = 0;
	for (int x = 6; x < 500; x++)
	{

		if (1 != sockRecv(connection, &buffer[x], 1))
		{
			sendErrorCode(POLYM_ERROR_CONNECTION_FAIL, connection);
			cleanupConnection(info);
			return POLYM_ERROR_CONNECTION_FAIL;
		}

		if (buffer[x] == '\r')
		{
			newlineTerminus = 1;
			continue;
		}
		else if (newlineTerminus == 1)
			if (buffer[x] == '\n')
			{
				newlineTerminus = 2;
				break;
			}
			else
				newlineTerminus = 0;
	}

	if (newlineTerminus != 2) // greeting is too long, close the connection
		closeUnitializedConnection(connection);

	insertShortIntoBuffer(buffer, 0); // encryption mode
	insertShortIntoBuffer(&buffer[2], 1); // realm code

	sockSend(connection, buffer, 4); // send the connection request

	if (2 != sockRecv(connection, buffer, 2))
	{
		sendErrorCode(POLYM_ERROR_CONNECTION_FAIL, connection);
		cleanupConnection(info);
		return POLYM_ERROR_CONNECTION_FAIL;
	}

	initializeNewPeerInfo(info, connection, out_connectionPointer);

	// TODO: send service update request and update the database

	return getShortFromBuffer(buffer);

}

void cleanupConnection(POLYM_CONNECTION_INFO *connection_info)
{
	switch (connection_info->mode)
	{

	case POLYM_MODE_SERVICE:
		// close a service control connection and all its auxiliary connections
		closeAllServiceAux(connection_info->mode_info.service.serviceID);
		removeService(connection_info->mode_info.service.serviceID);
		break;

	case POLYM_MODE_SERVICE_AUX:
		// close a service auxiliary connection
		removeServiceAux(connection_info->mode_info.serviceAux.serviceID, connection_info->mode_info.serviceAux.servicePort);
		break;

	case POLYM_MODE_PEER:
		// close a peer connection
		removePeer(connection_info->mode_info.peer.peerID);
		break;

	default:
		break;
	}
}

int connectServiceToPeer(POLYM_CONNECTION_INFO *peerInfo, POLYM_CONNECTION_INFO *serviceInfo)
{
	//check to ensure that there is room for this new connection, fail if not
	if (POLYM_PEER_MAX_SERVICE_CONNECTIONS <= int_array_count(&peerInfo->mode_status.peer.connectedServices))
		return POLYM_ERROR_PEER_MAX_SERVICE_CONNECTIONS; // this peer's service connections is full
	if (POLYM_SERVICE_MAX_PEER_CONNECTIONS <= int_array_count(&serviceInfo->mode_status.service.connectedPeers))
		return POLYM_ERROR_SERVICE_MAX_PEER_CONNECTIONS; // this services's peer connections is full

	int_array_push(&serviceInfo->mode_status.service.connectedPeers, serviceInfo->mode_info.service.serviceID);

	lockConnectionMutexByInfo(peerInfo);
	int_array_push(&peerInfo->mode_status.peer.connectedServices, peerInfo->mode_info.peer.peerID);
	unlockConnectionMutexByInfo(peerInfo);

	return 0;
}

int connectPeerToService(POLYM_CONNECTION_INFO *peerInfo, POLYM_CONNECTION_INFO *serviceInfo)
{
	//check to ensure that there is room for this new connection, fail if not
	if (POLYM_PEER_MAX_SERVICE_CONNECTIONS <= int_array_count(&peerInfo->mode_status.peer.connectedServices))
		return POLYM_ERROR_PEER_MAX_SERVICE_CONNECTIONS; // this peer's service connections is full
	if (POLYM_SERVICE_MAX_PEER_CONNECTIONS <= int_array_count(&serviceInfo->mode_status.service.connectedPeers))
		return POLYM_ERROR_SERVICE_MAX_PEER_CONNECTIONS; // this services's peer connections is full

	int_array_push(&peerInfo->mode_status.peer.connectedServices, peerInfo->mode_info.peer.peerID);

	lockConnectionMutexByInfo(serviceInfo);
	int_array_push(&serviceInfo->mode_status.service.connectedPeers, serviceInfo->mode_info.service.serviceID);
	unlockConnectionMutexByInfo(serviceInfo);

	return 0;
}