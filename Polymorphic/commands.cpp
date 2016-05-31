#include <commands.h>
#include <winsock2.h>

void processCommand(void* rawSocket, char* command, CONNECTION_INFO info)
{

	short commandLong = ntohs((((short)command[0]) << 8) | command[1]);

	// Command block

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