#include <connections.h>
#include <commands.h>
#include <definitions.h>

void sendPeerDisconnected(void* connection, uint16_t peerID)
{
	uint8_t buffer[4];

	insertShortIntoBuffer(buffer[0], POLY_COMMAND_CONNECT_ERROR);
	insertLongIntoBuffer(buffer[2], peerID);

	sockSendAsync(connection, buffer, 4);
}

int sendDisconnect(void *connection, POLYM_CONNECTION_INFO *connection_info, uint16_t disconnectCode)
{
	uint8_t buffer[4]; // create a buffer for operations

	insertShortIntoBuffer(buffer[0], POLY_COMMAND_DISCONNECT); // set command header to DISCONNECT
	insertShortIntoBuffer(buffer[2], disconnectCode); // add the disconnection code

	if (4 == sockSend(connection, buffer, 4)) // attempt to send the command
	{
		cleanupConnection(connection_info); // if successful, cleanup the connection and return no error
		return 0;
	}
	else
	{
		cleanupConnection(connection_info); // if unsuccessful, cleanup the connection and return NOT_GRACEFUL error
		return POLYM_ERROR_NOT_GRACEFUL;
	}
}

int sendConnectErrorToService(void* connection, uint32_t ipv4AddressLong, uint16_t l4Port, uint8_t protocol, uint16_t errorCode)
{
	uint8_t buffer[12];

	insertShortIntoBuffer(buffer[0], POLY_COMMAND_CONNECT_ERROR);
	insertLongIntoBuffer(buffer[2], ipv4AddressLong);
	insertShortIntoBuffer(buffer[6], l4Port);
	insertShortIntoBuffer(buffer[8], protocol);
	insertShortIntoBuffer(buffer[10], errorCode);

	sockSendAsync(connection, buffer, 12);
}

void recvConnect(void *connection, POLYM_CONNECTION_INFO *connection_info)
{

	uint8_t buffer[8];

	switch (connection_info->mode)
	{
	case POLYM_MODE_SERVICE:
		service:
		if (0 != trySockRecv(connection, buffer, 8))
			return;
		uint32_t ipv4AddressLong = getLongFromBuffer(buffer[0]);
		char ipv4Address[4];
		intIPtoStringIP(ipv4AddressLong, ipv4Address);
		uint16_t l4Port = getShortFromBuffer(buffer[4]);
		uint16_t protocol = getShortFromBuffer(buffer[6]);
		uint16_t connectResult = initializeOutgoingConnection(ipv4Address, l4Port, protocol);
		if (0 != connectResult)
			sendConnectErrorToService(connection, ipv4AddressLong, l4Port, protocol, connectResult);
		break;
	case POLYM_MODE_SERVICE_AUX:
		// same as service
		goto service;
	case POLYM_MODE_PEER:
		// peers should not be telling us to connect to other peers. disconnect.
		sendDisconnect(connection, connection_info, POLYM_ERROR_INVALID_COMMAND);
		return;
	}

}

int sendMessageToPeerID(uint16_t peerID, uint8_t *message, uint32_t length)
{
	void *connection = getConnectionFromPeerID(peerID);

	if (connection == NULL)
		return POLYM_ERROR_PEER_DOES_NOT_EXIST;

	sockSendAsync(connection, message, length);
	return 0;
}

int sendMessageErrorToService(void* connection, uint16_t peerID, uint16_t errorCode)
{
	uint8_t buffer[6];

	insertShortIntoBuffer(buffer[0], POLY_COMMAND_MESSAGE_ERROR);
	insertShortIntoBuffer(buffer[2], peerID);
	insertShortIntoBuffer(buffer[4], errorCode);

	sockSendAsync(connection, buffer, 6);
}

int sendMessageToServiceID(uint16_t serviceID, uint16_t portID, uint8_t* message, uint16_t length)
{
	void *connection = getConnectionFromServiceID(serviceID, portID);

	if (connection == NULL)
		return POLYM_ERROR_SERVICE_DOES_NOT_EXIST;

	sockSendAsync(connection, message, length);
	return 0;
}

int sendMessageErrorToPeer(void* connection, uint16_t serviceID, uint16_t servicePort, uint16_t errorCode)
{
	uint8_t buffer[8];

	insertShortIntoBuffer(buffer[0], POLY_COMMAND_MESSAGE_ERROR);
	insertShortIntoBuffer(buffer[2], serviceID);
	insertShortIntoBuffer(buffer[4], servicePort);
	insertShortIntoBuffer(buffer[6], errorCode);

	sockSendAsync(connection, buffer, 6);
}


// takes a connection and a partially filled out service to peer message, finishes it, and sends it.
void sendPeerMessage(void* connection, uint8_t* peerToPeerMessage)
{
	uint8_t buffer[2];

	if (0 != trySockRecv(connection, buffer, 2))
		return; // attempt to recieve destination peer ID count, return if fail.

	uint16_t destPeerID = getShortFromBuffer(buffer);

	if (0 != trySockRecv(connection, peerToPeerMessage[4], 6))
		return; // attempt to recieve the static header

	uint16_t messageLength = getShortFromBuffer(peerToPeerMessage[8]); // get the message feame length. so we can create an appropriately sized buffer

	if (0 != trySockRecv(connection, peerToPeerMessage[10], messageLength))
		return; // attempt to recieve messageLength, return if fail.

	uint32_t sendResult = sendMessageToPeerID(destPeerID, peerToPeerMessage, messageLength + 10);
	if (0 != sendResult)
		sendMessageErrorToService(connection, destPeerID, sendResult);
}

void recvMessage(void *connection, POLYM_CONNECTION_INFO *connection_info)
{

	switch (connection_info->mode)
	{

	case POLYM_MODE_SERVICE:
	{

		uint8_t peerToPeerMessage[65536 + 10]; // the max possible size of a peer->peer message command

		insertShortIntoBuffer(peerToPeerMessage, connection_info->mode_info.service.serviceID);
		peerToPeerMessage[2] = 0;
		peerToPeerMessage[3] = 0;

		sendPeerMessage(connection, peerToPeerMessage);

		break;
	}

	case POLYM_MODE_SERVICE_AUX:
	{
		uint8_t peerToPeerMessage[65536 + 10]; // the max possible size of a peer->peer message command

		insertShortIntoBuffer(peerToPeerMessage, connection_info->mode_info.serviceAux.serviceID);
		insertShortIntoBuffer(peerToPeerMessage[2], connection_info->mode_info.serviceAux.serviceID);

		sendPeerMessage(connection, peerToPeerMessage);
	}

	case POLYM_MODE_PEER:
	{

		uint8_t peerToServiceMessage[65536 + 8]; // the max possible size of a peer->service message command
		uint8_t buffer[4];

		if (0 != trySockRecv(connection, buffer, 4))
			return; // attempt to recieve the message command header

		uint16_t destServiceID = getShortFromBuffer(buffer);
		uint16_t destServicePort = getShortFromBuffer(buffer[2]); //get the destination from the header

		insertShortIntoBuffer(peerToServiceMessage, connection_info->mode_info.peer.peerID);

		if (0 != trySockRecv(connection, peerToServiceMessage[2], 6))
			return; // attempt to recieve the message command header

		uint16_t messageLength = getShortFromBuffer(peerToServiceMessage[6]);

		if (0 != trySockRecv(connection, peerToServiceMessage[8], messageLength))
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
		recvDisconnect(connection_info);

	default:
		break;
	}
}