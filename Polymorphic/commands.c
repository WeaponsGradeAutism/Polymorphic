#include <connections.h>
#include <definitions.h>
#include <stdint.h>
#include <info_structure.h>

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

void sendPeerDisconnected(void* connection, uint16_t peerID)
{
	uint8_t buffer[4];

	insertShortIntoBuffer(buffer, POLY_COMMAND_CONNECT_ERROR);
	insertLongIntoBuffer(&buffer[2], peerID);

	sockSendAsync(connection, buffer, 4);
}

int sendDisconnect(void *connection, uint16_t disconnectCode)
{
	uint8_t buffer[4]; // create a buffer for operations

	insertShortIntoBuffer(buffer, POLY_COMMAND_DISCONNECT); // set command header to DISCONNECT
	insertShortIntoBuffer(&buffer[2], disconnectCode); // add the disconnection code

	if (4 == sockSend(connection, buffer, 4)) // attempt to send the command
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
		sendDisconnect(connection, POLY_PROTO_ERROR_TRANSMISSION_FAIL);
		return POLY_PROTO_ERROR_TRANSMISSION_FAIL;
	}
}

void sendConnectErrorToService(void* connection, uint32_t address, uint16_t port, uint8_t protocol, uint16_t errorCode)
{
	uint8_t buffer[12];

	insertShortIntoBuffer(buffer, POLY_COMMAND_CONNECT_ERROR);
	insertLongIntoBuffer(&buffer[2], address);
	insertShortIntoBuffer(&buffer[6], port);
	insertShortIntoBuffer(&buffer[8], protocol);
	insertShortIntoBuffer(&buffer[10], errorCode);

	sockSendAsync(connection, buffer, 12);
}

/* connection procedure:
1. check to see if the peer/service in question is already connected
2. if requested service is not connected, stop and send error. if requested peer is not already connected, attempt to connect to it
3. associate the resource with the connecter
4. return the resource identifier
*/
void recvConnect(void *connection, POLYM_CONNECTION_INFO *connection_info)
{

	uint8_t buffer[8];

	switch (connection_info->realm)
	{
	case POLY_REALM_SERVICE:

		// recieve the connect info
		if (0 != trySockRecv(connection, buffer, 8))
			return;

		// convert the integer fields to local format
		uint32_t address = getLongFromBuffer(buffer);
		uint16_t port = getShortFromBuffer(&buffer[4]);
		uint8_t protocol = (uint8_t)getShortFromBuffer(&buffer[6]);

		// convert the integer address into string presentation
		char stringAddress[16];
		intIPtoStringIP(address, stringAddress, 16);

		//don't connect to a loopback address
		if (address >= 2130706433 && address <= 2147483646)
		{
			sendConnectErrorToService(connection, address, port, protocol, POLY_PROTO_ERROR_INVALID_ADDRESS);
			return;
		}

		// check to see if the peer is already connected
		int checkAddressConnected = addressConnected(stringAddress, port, protocol);
		if (checkAddressConnected != -1)
		{

		}
		void *newConnection;
		uint16_t connectResult = initializeOutgoingConnection(stringAddress, port, protocol, &newConnection);
		if (0 != connectResult)
			sendConnectErrorToService(connection, address, port, protocol, connectResult);
		break;
	case POLY_REALM_PEER:
		// peers should not be telling us to connect to other peers. this is an error. disconnect.
		sendDisconnect(connection, POLY_PROTO_ERROR_INVALID_COMMAND);
		return;
	case POLY_REALM_CLIENT:
		// peers should not be telling us to connect to other peers. this is an error. disconnect.
		sendDisconnect(connection, POLY_PROTO_ERROR_INVALID_COMMAND);
		return;
	}

}

void sendMessageErrorToService(void* connection, uint16_t peerID, uint16_t errorCode)
{
	uint8_t buffer[6];

	insertShortIntoBuffer(buffer, POLY_COMMAND_MESSAGE_ERROR);
	insertShortIntoBuffer(&buffer[2], peerID);
	insertShortIntoBuffer(&buffer[4], errorCode);

	sockSendAsync(connection, buffer, 6);
}

int sendMessageToServiceID(uint16_t serviceID, uint16_t portID, uint8_t* message, uint16_t length)
{
	void *connection = getConnectionFromServiceID(serviceID);

	if (connection == NULL)
		return POLY_PROTO_ERROR_SERVICE_DOES_NOT_EXIST;

	sockSendAsync(connection, message, length);
	return 0;
}

void sendMessageErrorToPeer(void* connection, uint16_t serviceID, uint16_t servicePort, uint16_t errorCode)
{
	uint8_t buffer[8];

	insertShortIntoBuffer(buffer, POLY_COMMAND_MESSAGE_ERROR);
	insertShortIntoBuffer(&buffer[2], serviceID);
	insertShortIntoBuffer(&buffer[4], servicePort);
	insertShortIntoBuffer(&buffer[6], errorCode);

	sockSendAsync(connection, buffer, 6);
}

void sendMessageIn(uint16_t destID, uint8_t *message, int totalLength)
{
	void *connection;
	connection = getConnectionFromServiceID(destID);
	if (connection == NULL)
		return; //TODO: Send error code

	sockSendAsync(connection, message, totalLength);
}

void sendMessageOut(uint16_t peerID, uint8_t *message, int totalLength)
{
	void *connection;
	connection = getConnectionFromPeerID(peerID);
	if (connection == NULL)
		return; //TODO: Send error code

	sockSendAsync(connection, message, totalLength);
}

void recvMessage(void *connection, POLYM_CONNECTION_INFO *connection_info)
{

	switch (connection_info->realm)
	{

	case POLY_REALM_SERVICE:
	{

		//TODO: add proper failure responses
		uint8_t message[POLY_COMMAND_MESSAGE_MAX_SIZE]; // the max possible size of a message command
		uint16_t peerID, messageLength;

		if (0 != trySockRecv(connection, &message[POLY_COMMAND_OFFSET], 8)) return;
		else
		{
			peerID = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_OFFSET_PEER_ID]);
			messageLength = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH]);
		}
		insertShortIntoBuffer(message, POLY_COMMAND_MESSAGE);
		insertShortIntoBuffer(&message[POLY_COMMAND_MESSAGE_OFFSET_SOURCE_ID], connection_info->realm_info.service.serviceID);
		if (0 != trySockRecv(connection, &message[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE], messageLength)) return;

		sendMessageOut(peerID, message, 10 + messageLength);

		break;
	}

	case POLY_REALM_PEER:
	{

		//TODO: add proper failure responses
		uint8_t message[POLY_COMMAND_MESSAGE_MAX_SIZE]; // the max possible size of a message command
		uint16_t destID, messageLength;

		if (0 != trySockRecv(connection, &message[POLY_COMMAND_OFFSET], 8)) return;
		else
		{
			destID = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_OFFSET_DESTINATION_ID]);
			messageLength = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH]);
		}
		insertShortIntoBuffer(message, POLY_COMMAND_MESSAGE);
		insertShortIntoBuffer(&message[POLY_COMMAND_MESSAGE_OFFSET_PEER_ID], connection_info->realm_info.peer.peerID);
		if (0 != trySockRecv(connection, &message[POLY_COMMAND_MESSAGE_OFFSET_MESSAGE], messageLength)) return;

		sendMessageIn(destID, message, 10 + messageLength);

		break;
	}

	case POLY_REALM_CLIENT:
	{

		// TODO: This realm cannot use this command. Send error.

	}

	default:
		break;
	}
}

void sendMessageService(uint16_t destID, uint8_t *message, int totalLength)
{
	void *connection;
	connection = getConnectionFromServiceID(destID);
	if (connection == NULL)
		return; //TODO: Send error code.

	sockSendAsync(connection, message, totalLength);
}

void recvMessageService(void *connection, POLYM_CONNECTION_INFO *connection_info)
{
	switch (connection_info->realm)
	{

	case POLY_REALM_SERVICE:
	{

		//TODO: add proper failure responses
		uint8_t message[POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE]; // the max possible size of a message command
		uint16_t destID, messageLength;

		if (0 != trySockRecv(connection, &message[POLY_COMMAND_OFFSET], 6)) return;
		else
		{
			destID = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_DESTINATION_ID]);
			messageLength = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_LENGTH]);
		}
		insertShortIntoBuffer(message, POLY_COMMAND_MESSAGE_SERVICE);
		insertShortIntoBuffer(&message[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID], connection_info->realm_info.service.serviceID);
		if (0 != trySockRecv(connection, &message[POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE], messageLength)) return;

		sendMessageService(destID, message, 8 + messageLength);

		break;
	}

	case POLY_REALM_PEER:
	{

		// TODO: This realm cannot use this command. Send error.

	}

	case POLY_REALM_CLIENT:
	{

		// TODO: This realm cannot use this command. Send error.

	}

	default:
		break;
	}
}

void sendMessageClientIn(uint16_t destID, uint8_t *message, int totalLength)
{
	void *connection;
	connection = getConnectionFromClientID(destID);
	if (connection == NULL)
		return; //TODO: Send error code.

	sockSendAsync(connection, message, totalLength);
}

void sendMessageClientOut(uint16_t destID, uint8_t *message, int totalLength)
{
	void *connection;
	connection = getConnectionFromServiceID(destID);
	if (connection == NULL)
		return; //TODO: Send error code.

	sockSendAsync(connection, message, totalLength);
}

void recvMessageClient(void *connection, POLYM_CONNECTION_INFO *connection_info)
{

	switch (connection_info->realm)
	{
	case POLY_REALM_SERVICE:
	{
		//TODO: add proper failure responses
		uint8_t message[POLY_COMMAND_MESSAGE_CLIENT_MAX_SIZE]; // the max possible size of a message command
		uint16_t destID, messageLength;

		if (0 != trySockRecv(connection, &message[POLY_COMMAND_OFFSET], 6)) return;
		else
		{
			destID = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID]);
			messageLength = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH]);
		}
		insertShortIntoBuffer(message, POLY_COMMAND_MESSAGE_CLIENT);
		insertShortIntoBuffer(&message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID], connection_info->realm_info.service.serviceID);
		if (0 != trySockRecv(connection, &message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE], messageLength)) return;

		sendMessageClientIn(destID, message, 8 + messageLength);
		break;

	}
	case POLY_REALM_CLIENT:
	{
		//TODO: add proper failure responses
		uint8_t message[POLY_COMMAND_MESSAGE_CLIENT_MAX_SIZE]; // the max possible size of a message command
		uint16_t destID, messageLength;

		if (0 != trySockRecv(connection, &message[POLY_COMMAND_OFFSET], 6)) return;
		else
		{
			destID = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID]);
			messageLength = getShortFromBuffer(&message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH]);
		}
		insertShortIntoBuffer(message, POLY_COMMAND_MESSAGE_CLIENT);
		insertShortIntoBuffer(&message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID], connection_info->realm_info.client.clientID);
		if (0 != trySockRecv(connection, &message[POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE], messageLength)) return;

		sendMessageClientOut(destID, message, 8 + messageLength);
		break;
	}
	}
}

// big, dumb switch block
void processCommand(void *connection, uint8_t *command, POLYM_CONNECTION_INFO *connection_info)
{

	uint16_t commandLong = getShortFromBuffer(command);

	switch (commandLong)
	{

	case POLY_COMMAND_CONNECT:
		// CONNECT
		break;
	case POLY_COMMAND_MESSAGE:
		recvMessage(connection, connection_info);
		break;
	case POLY_COMMAND_MESSAGE_SERVICE:
		recvMessageService(connection, connection_info);
		break;
	case POLY_COMMAND_DISCONNECT:
		//recvDisconnect(connection_info);
		break;

	default:
		break;
	}
}