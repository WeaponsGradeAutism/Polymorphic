#pragma once

#include <ws2tcpip.h>
#include <stdint.h>
#include <info_structure.h>
#include <definitions.h>

#define POLYM_SERVICE_ARRAY_GETALL_OVERFLOW -1

typedef struct {
	DWORD threadID;
	HANDLE threadHandle;
} THREAD_INFO;

typedef struct _POLYM_OVERLAPPED {
	ULONG_PTR Internal;
	ULONG_PTR InternalHigh;
	union {
		struct {
			DWORD Offset;
			DWORD OffsetHigh;
		} DUMMYSTRUCTNAME;
		PVOID Pointer;
	} DUMMYUNIONNAME;

	HANDLE  hEvent;
	int eventType;
	void* eventInfo;
} POLYM_OVERLAPPED, *LPPOLYM_OVERLAPPED;

// Connection struct. Houses connection state and various data structures required for IOCP to function.
// All datas in this struct is immutible. It should never be changed after connection initialization.
// This means that this data is thread-safe and should not need synchronization.
typedef struct {
	SOCKET socket; // the actual socket handle itself. the star of the show

	union address {
		struct sockaddr_in ipv4;
		struct sockaddr_in6 ipv6;
	} address;
	uint8_t protocol; // the layer-4 protocol used for this connection
	uint8_t addrtype; //IPPROTO_IPV4 or IPPROTO_IPV6
	uint8_t encryptionType; // kind of encryption key used for this connection
	uint8_t encryptionKey[32]; // the encryption key used to decrypt traffic

				   // connection information
	POLYM_CONNECTION_INFO info; // info struct, containing info shared with the commands.c file. defnition in commands header file.
	

				   // connection internals
	HANDLE checkAliveTimer; // the timer that periodically checks to see if the connection is still alive
	HANDLE connectionMutex; // synchronizes access to this connection. used to sync both i/o use and status reads and writes

							// IOCP internals
	int32_t byteCount; // bytecount for listening event
	int32_t flags; // used for IOCP listening event
	WSABUF buffer; // buffer where IOCP data is stored before completion packet is passed for listening event
	uint8_t bufferMemory[2]; // memory storage for the buf element of WSABUF
	POLYM_OVERLAPPED overlap; // reused overlapped info for listening event

	int index; // used to keep track of position in connection array

	
} POLYM_CONNECTION; //TODO: rename to POLYM_CONNECTION

typedef struct {
	POLYM_CONNECTION* connection;
	int event;
	void* eventInfo;
} CONNECTION_EVENT;

typedef struct {
	POLYM_CONNECTION **connections; // pointer to a POLYM_CONNECTION* array
	int size; // the current size of the message array
	int count; // the number of messages currently on the array
	int nextIndex; // the next index to scan for openings in the array
} connection_array;

typedef struct {
	WSABUF wsabuf;
	POLYM_OVERLAPPED overlap;
	POLYM_MESSAGE_BUFFER buffer_object; // largest possible POLY message
	int index; // this message's position in the message array, set to -1 if currently not in use
} message_buffer;

typedef struct {
	message_buffer **messages; // pointer to a message_buffer* array
	int size; // the current size of the message array
	int count; // the number of messages currently on the array
	int nextIndex; // the next index to scan for openings in the array
} message_buffer_array;

void connection_array_init(connection_array *vector);
void connection_array_init_capacity(connection_array *vector, int size);
int connection_array_push(connection_array *vector, POLYM_CONNECTION *connection, void **out_connectionPointer);
POLYM_CONNECTION* connection_array_get(connection_array *vector, int index);
int connection_array_get_all(connection_array *vector, int maxCount, POLYM_CONNECTION **OUT_connectionArray);
void connection_array_free(connection_array *vector, int index);
void connection_array_free_container(connection_array *vector);

void message_buffer_init(message_buffer *buffer);

void message_buffer_array_init(message_buffer_array *vector, int size);
message_buffer* message_buffer_array_allocate(message_buffer_array *vector);
void message_buffer_array_free(message_buffer_array *vector, int index);
void message_buffer_array_free_container(message_buffer_array *vector);
