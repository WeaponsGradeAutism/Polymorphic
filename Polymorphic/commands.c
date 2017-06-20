#include <connections.h>
#include <definitions.h>
#include <stdint.h>
#include <info_structure.h>
#include <utils.h>
#include <sock.h>

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

int sendDisconnect(void *connection, uint16_t disconnectCode)
{
	uint8_t buffer[POLY_COMMAND_DISCONNECT_MAX_SIZE]; // create a buffer for operations

	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_DISCONNECT); // set command header to DISCONNECT
	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], 0);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_DISCONNECT_CODE], disconnectCode); // add the disconnection code

	if (sockSend(connection, buffer, 4) == POLY_COMMAND_DISCONNECT_MAX_SIZE) // attempt to send the command
	{
		POLYM_CONNECTION_INFO *connection_info = getInfoFromConnection(connection);
		removeConnection(connection_info); // if successful, cleanup the connection and return no error
		return 0;
	}
	else
	{
		POLYM_CONNECTION_INFO *connection_info = getInfoFromConnection(connection);
		removeConnection(connection_info); // if unsuccessful, cleanup the connection and return NOT_GRACEFUL error
		return POLYM_ERROR_NOT_GRACEFUL;
	}
}

int trySockRecv(void *connection, uint8_t* buffer, uint32_t length)
{
	if (length == sockRecv(connection, buffer, length))
		return 0;
	else
	{
		// send error message and disconnect
		sendDisconnect(connection, POLY_ERROR_TRANSMISSION_FAIL);
		return POLY_ERROR_TRANSMISSION_FAIL;
	}
}

void sendError(void* connection, uint16_t errorCode, uint16_t indentifier)
{
	uint8_t *buffer[POLY_COMMAND_ERROR_MAX_SIZE];

	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_ERROR);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], indentifier);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_ERROR_OFFSET_ERROR_CODE], errorCode);

	sockSend(connection, buffer, POLY_COMMAND_ERROR_MAX_SIZE);
}

void sendErrorToPeer(uint16_t peerID, uint16_t errorCode, uint16_t indentifier)
{
	sendError(getConnectionFromPeerID(peerID), errorCode, indentifier);
}

void sendErrorToService(uint16_t serviceID, uint16_t errorCode, uint16_t indentifier)
{
	sendError(getConnectionFromServiceID(serviceID), errorCode, indentifier);
}

void sendErrorToClient(uint16_t clientID, uint16_t errorCode, uint16_t indentifier)
{
	sendError(getConnectionFromClientID(clientID), errorCode, indentifier);
}

void sendPeerDisconnected(void* connection, uint16_t peerID)
{

	uint8_t *buffer[POLY_COMMAND_PEER_DISCONNECTED_MAX_SIZE];

	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_CONNECT_ERROR);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], 0);
	insertLongIntoBuffer(&buffer[POLY_COMMAND_PEER_DISCONNECTED_OFFSET_PEER_ID], peerID);

	sockSend(connection, buffer, POLY_COMMAND_PEER_DISCONNECTED_MAX_SIZE);
}

void sendConnectErrorToService(void* connection, uint32_t address, uint16_t port, uint8_t protocol, uint16_t errorCode)
{
	int8_t *buffer[POLY_COMMAND_CONNECT_ERROR_MAX_SIZE];

	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_CONNECT_ERROR);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], 0);
	insertLongIntoBuffer(&buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS], address);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT], port);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL], protocol);
	insertShortIntoBuffer(&buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_ERROR_CODE], errorCode);

	sockSend(connection, buffer, POLY_COMMAND_CONNECT_ERROR_MAX_SIZE);
}

/* connection procedure:
1. check to see if the peer/service in question is already connected
2. if requested service is not connected, stop and send error. if requested peer is not already connected, attempt to connect to it
3. associate the resource with the connecter
4. return the resource identifier
*/
void recvConnect(void *connection, POLYM_CONNECTION_INFO *connection_info, uint16_t identifier)
{

	uint8_t buffer[POLY_COMMAND_CONNECT_MAX_SIZE];

	switch (connection_info->realm)
	{
	case POLY_REALM_SERVICE:

		// recieve the connect info
		if (0 != trySockRecv(connection, buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS], POLY_COMMAND_CONNECT_MAX_SIZE - 2))
			return;

		rearmListenSocket(connection);

		// convert the integer fields to local format
		uint32_t address = getLongFromBuffer(buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS]);
		uint16_t port = getShortFromBuffer(&buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT]);
		uint8_t protocol = (uint8_t)getShortFromBuffer(&buffer[POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL]);

		connectServiceToPeer(connection_info, address, port, protocol);

		break;

	case POLY_REALM_PEER:

		rearmListenSocket(connection);
		sendError(connection, POLY_ERROR_DISALLOWED_COMMAND, identifier);
		break;

	case POLY_REALM_CLIENT:

		rearmListenSocket(connection);
		sendError(connection, POLY_ERROR_DISALLOWED_COMMAND, identifier);
		break;

	}

}

int sendMessageIn(uint16_t destID, uint8_t *buffer, int length)
{
	void *connection = getConnectionFromServiceID(destID);
	if (connection == NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	sockSend(connection, buffer, length);
	return 0;
}

int sendMessageOut(uint16_t peerID, uint8_t *buffer, int length)
{
	void *connection = getConnectionFromPeerID(peerID);
	if (connection == NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	sockSend(connection, buffer, length);
	return 0;
}

void recvMessage(void *connection, POLYM_CONNECTION_INFO *connection_info, uint16_t identifier)
{

	switch (connection_info->realm)
	{

	case POLY_REALM_SERVICE:
	{
 
		uint8_t buffer[POLY_COMMAND_MESSAGE_MAX_SIZE];
		uint16_t peerID, messageLength;
		int messageSize;

		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_MESSAGE_OFFSET_PEER_ID], POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH - POLY_COMMAND_MESSAGE_OFFSET_PEER_ID))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}
		else
		{
			messageLength = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH]);
		}
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE], messageLength))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}

		rearmListenSocket(connection);

		peerID = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_OFFSET_PEER_ID]);

		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_MESSAGE);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], identifier);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_MESSAGE_OFFSET_SOURCE_ID], connection_info->connectionID);

		messageSize = messageLength + POLY_COMMAND_MESSAGE_OFFSET_MESSAGE;

		int result = sendMessageOut(peerID, buffer, messageSize);
		if (result != 0)
			sendError(connection, result, identifier);

		break;
	}

	case POLY_REALM_PEER:
	{

		uint8_t buffer[POLY_COMMAND_MESSAGE_MAX_SIZE];
		uint16_t destID, messageLength;
		int messageSize;

		
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_HEADER_SIZE], POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH - POLY_COMMAND_MESSAGE_OFFSET_PEER_ID))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}
		else
		{
			
			messageLength = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH]);
		}
		
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE], messageLength))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}

		rearmListenSocket(connection);

		destID = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_OFFSET_DESTINATION_ID]);

		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_MESSAGE);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], identifier);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_MESSAGE_OFFSET_PEER_ID], connection_info->connectionID);

		messageSize = messageLength + POLY_COMMAND_MESSAGE_OFFSET_MESSAGE;

		int result = sendMessageIn(destID, buffer, messageLength);
		if (result != 0)
			sendError(connection, result, identifier);

		break;
	}

	case POLY_REALM_CLIENT:
	{

		rearmListenSocket(connection);
		sendError(connection, POLY_ERROR_DISALLOWED_COMMAND, identifier);
		break;

	}

	default:
		rearmListenSocket(connection);
		break;
	}
}

int sendMessageService(uint16_t destID, uint8_t *buffer, int length)
{
	void *connection = getConnectionFromServiceID(destID);
	if (connection == NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	sockSend(connection, buffer, length);
	return 0;
}

void recvMessageService(void *connection, POLYM_CONNECTION_INFO *connection_info, uint16_t identifier)
{
	switch (connection_info->realm)
	{

	case POLY_REALM_SERVICE:
	{

		uint8_t buffer[POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE];
		uint16_t destID, messageLength;
		int messageSize;
		
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID], POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_LENGTH - POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}
		else
		{
			messageLength = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_LENGTH]);
		}
		
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE], messageLength))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}

		rearmListenSocket(connection);

		destID = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_DESTINATION_ID]);

		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_MESSAGE_SERVICE);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], identifier);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID], connection_info->connectionID);

		messageSize = messageLength + POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE;

		int result = sendMessageService(destID, buffer, messageSize);
		if (result != 0)
			sendError(connection, result, identifier);

		break;
	}

	case POLY_REALM_PEER:
	{

		rearmListenSocket(connection);
		sendError(connection, POLY_ERROR_DISALLOWED_COMMAND, identifier);
		break;

	}

	case POLY_REALM_CLIENT:
	{

		rearmListenSocket(connection);
		sendError(connection, POLY_ERROR_DISALLOWED_COMMAND, identifier);
		break;

	}

	default:
		rearmListenSocket(connection);
		break;
	}
}

int sendMessageClientIn(uint16_t destID, uint8_t *buffer, int length)
{
	void *connection = getConnectionFromClientID(destID);
	if (connection == NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	sockSend(connection, buffer, length);
	return 0;
}

int sendMessageClientOut(uint16_t destID, uint8_t *buffer, int length)
{
	void *connection = getConnectionFromServiceID(destID);
	if (connection == NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	sockSend(connection, buffer, length);
	return 0;
}

void recvMessageClient(void *connection, POLYM_CONNECTION_INFO *connection_info, uint16_t identifier)
{

	switch (connection_info->realm)
	{

	case POLY_REALM_PEER:
	{

		rearmListenSocket(connection);
		sendError(connection, POLY_ERROR_DISALLOWED_COMMAND, identifier);
		break;

	}

	case POLY_REALM_SERVICE:
	{
		uint8_t buffer[POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE];
		uint16_t destID, messageLength;
		int messageSize;

		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_HEADER_SIZE], POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}
		else
		{
			messageLength = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH]);
		}
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE], messageLength))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}

		rearmListenSocket(connection);

		destID = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID]);

		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_MESSAGE_CLIENT);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], identifier);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID], connection_info->connectionID);

		messageSize = messageLength + POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE;

		int result = sendMessageClientIn(destID, buffer, messageSize);
		if (result != 0)
			sendError(connection, result, identifier);

		break;

	}
	case POLY_REALM_CLIENT:
	{
		uint8_t buffer[POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE];
		uint16_t destID, messageLength;
		int messageSize;
		
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_HEADER_SIZE], POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}
		else
		{
			messageLength = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH]);
		}
		if (0 != trySockRecv(connection, &buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE], messageLength))
		{
			rearmListenSocket(connection);
			sendError(connection, POLY_ERROR_TRANSMISSION_FAIL, identifier);
			return;
		}

		rearmListenSocket(connection);

		destID = getShortFromBuffer(&buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID]);

		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_CODE], POLY_COMMAND_MESSAGE_CLIENT);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_HEADER_ID], identifier);
		insertShortIntoBuffer(&buffer[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID], connection_info->connectionID);

		messageSize = messageLength + POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE;

		int result = sendMessageClientOut(destID, buffer, messageSize);
		if (result != 0)
			sendError(connection, result, identifier);

		break;
	}

	default:
		rearmListenSocket(connection);
		break;

	}
}

// big, dumb switch block
void processCommand(void *connection, uint8_t *buffer, POLYM_CONNECTION_INFO *connection_info)
{

	uint16_t command = getShortFromBuffer(&buffer[POLY_COMMAND_HEADER_CODE]);
	uint16_t identifier = getShortFromBuffer(&buffer[POLY_COMMAND_HEADER_ID]);

	switch (command)
	{

	case POLY_COMMAND_CONNECT:
		recvConnect(connection, connection_info, identifier);
		break;
	case POLY_COMMAND_MESSAGE:
		recvMessage(connection, connection_info, identifier);
		break;
	case POLY_COMMAND_MESSAGE_SERVICE:
		recvMessageService(connection, connection_info, identifier);
		break;
	case POLY_COMMAND_MESSAGE_CLIENT:
		recvMessageClient(connection, connection_info, identifier);
		break;
	case POLY_COMMAND_DISCONNECT:
		//recvDisconnect(connection_info);
		break;
	case POLY_COMMAND_ERROR:
		// ERROR
		break;

	default:
		break;
	}
}