#include <commands.h>

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

int initializeConnection(void *socket, CONNECTION_INFO *connection_info)
{
	char* buffer[4];

	if (2 != sockRecv(socket, connection_info->protocol, buffer, 2))
		return POLY_MODE_FAILED;

	if (ntohs((((short)buffer[0]) << 8) | (short)buffer[1]) != 0)
	{
		printf("Encryption not yet supported.\n");
		return POLY_MODE_FAILED;
	}

	sockSend(socket, connection_info->protocol, "POLY v0.1\r\n", 11, 0);

	if (2 != sockRecv(socket, connection_info->protocol, buffer, 2))
		return POLY_MODE_FAILED;

	if (ntohs((((short)buffer[0]) << 8) | (short)buffer[1]) == 0)
	{
		if (2 != sockRecv(socket, connection_info->protocol, buffer, 2))
			return POLY_MODE_FAILED;

		if (ntohs((((short)buffer[0]) << 8) | (short)buffer[1]) == 0)
		{
			connection_info->mode = POLY_MODE_SERVICE;
			int serviceStringSize = ntohs((((short)buffer[2]) << 8) | (short)buffer[3]);
			connection_info->serviceString = malloc(sizeof(char) * serviceStringSize);
			RAND_bytes(connection_info->serviceKey, 4);
			if (serviceStringSize != sockRecv(socket, connection_info->protocol, connection_info->serviceString, serviceStringSize))
				return POLY_MODE_FAILED;

			sockSend(socket, connection_info->protocol, connection_info->serviceKey, 4, 0);
			return POLY_MODE_SERVICE_NEW;
		} 
		else
		{
			connection_info->mode = POLY_MODE_SERVICE;
			return POLY_MODE_SERVICE;
		}
	}
	else
	{
		connection_info->mode = POLY_MODE_PEER;
		return POLY_MODE_PEER;
	}
}

void processCommand(void *socket, char *command, CONNECTION_INFO *connection_info)
{

	short commandLong = ntohs((((short)command[0]) << 8) | command[1]);

	sockSend(socket, connection_info->protocol, "HELLO!\n", 7, 0);

	// big, dumb switch block
	switch (commandLong)
	{
	case 0x00:
		// CONNECT
	case 0x01:
		// CONNECTv6
	case 0x02:
		// NEW-CONNECTION (should not be recieved here!)
	case 0x03:
		// NEW-CONNECTIONv6 (should not be recieved here!)
	case 0x04:
		// PING
	case 0x05:
		// MESSAGE
	case 0x06:
		// MESSAGE-SAFE
	case 0x07:
		// PEER-LIST
	case 0x08:
		// PEER-INFO
	case 0x09:
		// BROADCAST-PEER
	case 0x0A:
		// SUBSCRIBE-PEER
	case 0xAA:
		// NEW_PEER
	case 0x0B:
		// FIND-SERVICE
	case 0x0C:
		// BROADCAST-SERVICES
	case 0x0D:
		// REQUEST-SERVICES
	case 0x0E:
		// DEREGISTER-SERVICE
	case 0x0F:
		// REPLACE-SERVICE
	case 0x10:
		// NEW-SERVICE
	case 0xAB:
		// FIND-USER
	case 0x11:
		// BROADCAST-USER
	case 0x12:
		// REQUEST-USERS
	case 0x13:
		// CREATE-USER
	case 0x14:
		// SERVICE-OFFLINE
	case 0x15:
		// SERVICE-NOT-FOUND


	default:
		break;
	}
}