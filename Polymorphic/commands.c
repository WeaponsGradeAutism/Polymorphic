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

	switch (connection_info->mode)
	{
	case POLY_MODE_SERVICE:

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
	case POLY_MODE_PEER:
		// peers should not be telling us to connect to other peers. this is an error. disconnect.
		sendDisconnect(connection, POLY_PROTO_ERROR_INVALID_COMMAND);
		return;
	}

}

int sendMessageToPeerID(uint16_t peerID, uint8_t *message, uint32_t length)
{
	void *connection = getConnectionFromPeerID(peerID);

	if (connection == NULL)
		return POLY_PROTO_ERROR_PEER_DOES_NOT_EXIST;

	sockSendAsync(connection, message, length);
	return 0;
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


// takes a connection and a partially filled out service to peer message, finishes it, and sends it.
void sendPeerMessage(void* connection, uint8_t* peerToPeerMessage)
{
	uint8_t buffer[2];

	if (0 != trySockRecv(connection, buffer, 2))
		return; // attempt to recieve destination peer ID count, return if fail.

	uint16_t destPeerID = getShortFromBuffer(buffer);

	if (0 != trySockRecv(connection, &peerToPeerMessage[4], 6))
		return; // attempt to recieve the static header

	uint16_t messageLength = getShortFromBuffer(&peerToPeerMessage[8]); // get the message feame length. so we can create an appropriately sized buffer

	if (0 != trySockRecv(connection, &peerToPeerMessage[10], messageLength))
		return; // attempt to recieve messageLength, return if fail.

	uint32_t sendResult = sendMessageToPeerID(destPeerID, peerToPeerMessage, messageLength + 10);
	if (0 != sendResult)
		sendMessageErrorToService(connection, destPeerID, sendResult);
}

void recvMessage(void *connection, POLYM_CONNECTION_INFO *connection_info)
{

	switch (connection_info->mode)
	{

	case POLY_MODE_SERVICE:
	{

		uint8_t peerToPeerMessage[65536 + 10]; // the max possible size of a peer->peer message command

		insertShortIntoBuffer(peerToPeerMessage, connection_info->mode_info.service.serviceID);
		peerToPeerMessage[2] = 0;
		peerToPeerMessage[3] = 0;

		sendPeerMessage(connection, peerToPeerMessage);

		break;
	}

	case POLY_MODE_PEER:
	{

		uint8_t peerToServiceMessage[65536 + 8]; // the max possible size of a peer->service message command
		uint8_t buffer[4];

		if (0 != trySockRecv(connection, buffer, 4))
			return; // attempt to recieve the message command header

		uint16_t destServiceID = getShortFromBuffer(buffer);
		uint16_t destServicePort = getShortFromBuffer(&buffer[2]); //get the destination from the header

		insertShortIntoBuffer(peerToServiceMessage, connection_info->mode_info.peer.peerID);

		if (0 != trySockRecv(connection, &peerToServiceMessage[2], 6))
			return; // attempt to recieve the message command header

		uint16_t messageLength = getShortFromBuffer(&peerToServiceMessage[6]);

		if (0 != trySockRecv(connection, &peerToServiceMessage[8], messageLength))
			return; // attempt to recieve the initial message frame in the message command

		int32_t sendResult = sendMessageToServiceID(destServiceID, destServicePort, peerToServiceMessage, messageLength + 8);
		if (0 != sendResult)
			sendMessageErrorToPeer(connection, destServiceID, destServicePort, sendResult);

		break;
	}

	default:
		break;
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
	case POLY_COMMAND_DISCONNECT:
		//recvDisconnect(connection_info);

	default:
		break;
	}
}