#include <utils.h>
#include <ws2tcpip.h>

///<summary> Inserts the provided short into the provided buffer, converting it to network format. </summary>
void insertShortIntoBuffer(uint8_t *buffer, uint16_t unconvertedShort)
{
	uint16_t convertedShort = htons(unconvertedShort);
	buffer[0] = convertedShort >> 8;
	buffer[1] = convertedShort & 0x00FF;
}

///<summary> Inserts the provided short into the provided buffer, but does not convert it. </summary>
void insertShortIntoBufferNC(uint8_t *buffer, uint16_t unconvertedShort)
{
	buffer[0] = unconvertedShort >> 8;
	buffer[1] = unconvertedShort & 0x00FF;
}

///<summary> Inserts the provided long into the provided buffer, converting it to network format. </summary>
void insertLongIntoBuffer(uint8_t *buffer, uint32_t unconvertedLong)
{
	uint32_t convertedLong = htonl(unconvertedLong);
	buffer[0] = convertedLong >> 24;
	buffer[1] = (convertedLong >> 16) & 0x000000FF;
	buffer[2] = (convertedLong >> 8) & 0x000000FF;
	buffer[3] = convertedLong & 0x000000FF;
}

///<summary> Inserts the provided long into the provided buffer, but does not convert it. </summary>
void insertLongIntoBufferNC(uint8_t *buffer, uint32_t unconvertedLong)
{
	buffer[0] = unconvertedLong >> 24;
	buffer[1] = (unconvertedLong >> 16) & 0x000000FF;
	buffer[2] = (unconvertedLong >> 8) & 0x000000FF;
	buffer[3] = unconvertedLong & 0x000000FF;
}

///<summary> Gets the short from the provided buffer, converting it to network format. </summary>
uint16_t getShortFromBuffer(uint8_t* buffer)
{
	return ntohs((((uint16_t)buffer[0]) << 8) | buffer[1]);
}

///<summary> Gets the short from the provided buffer, but does not convert it. </summary>
uint16_t getShortFromBufferNC(uint8_t* buffer)
{
	return (((uint16_t)buffer[0]) << 8) | buffer[1];
}

///<summary> Gets the long from the provided buffer, converting it to network format. </summary>
uint32_t getLongFromBuffer(uint8_t* buffer)
{
	return ntohs((((uint32_t)buffer[0]) << 24) | buffer[1] << 16 | buffer[2] << 8 | buffer[3]);
}

///<summary> Gets the long from the provided buffer, but does not convert it. </summary>
uint32_t getLongFromBufferNC(uint8_t* buffer)
{
	return (((uint32_t)buffer[0]) << 24) | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

///<summary> Converts an integer ipv4 representation to its string representation. </summary>
const char* intIPtoStringIP(uint32_t ipv4AddressLong, char *out_StringIP, int bufferSize)
{
	struct in_addr addressOjbect;
	addressOjbect.S_un.S_addr = ipv4AddressLong;
	return inet_ntop(AF_INET, &addressOjbect, out_StringIP, bufferSize);
}
