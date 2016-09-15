#pragma once

#include <ws2tcpip.h>
#include <stdint.h>
#include <info_structure.h>

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

				   // connection information
	POLYM_CONNECTION_INFO info; // info struct, containing info shared with the commands.c file. defnition in commands header file.
	

				   // connection internals
	HANDLE checkAliveTimer; // the timer that periodically checks to see if the connection is still alive
	HANDLE connectionMutex; // synchronizes access to this connection. used to sync both i/o use and status reads and writes

							// IOCP internals
	int32_t byteCount; // bytecount for listening event
	int32_t flags; // used for IOCP listening event
	WSABUF buffer; // buffer where IOCP data is stored before completion packet is passed for listening event
	POLYM_OVERLAPPED overlap; // reused overlapped info for listening event

	union address {
		struct sockaddr_in ipv4;
		struct sockaddr_in6 ipv6;
	} address;
} CONNECTION;

typedef struct {
	CONNECTION* connection;
	int event;
	void* eventInfo;
} CONNECTION_EVENT;

typedef struct {
	uint32_t size;      // bound of the internal array
	uint32_t members;   // the number of used slots
	uint32_t capacity;  // total available slots
	CONNECTION **data;     // array of connections we're storing
} connection_vector;

typedef struct {
	connection_vector connections;
	int_vector vacancies;
} connection_array;

typedef struct {
	uint32_t size;      // bound of the internal array
	uint32_t members;   // the number of used slots
	uint32_t capacity;  // total available slots
	CONNECTION **data;     // array of connections we're storing
	connection_array **auxConnectionContainers; // array of aux connections containers
} service_connection_vector;

typedef struct {
	service_connection_vector connections;
	int_vector vacancies;
} service_connection_array;

int connection_array_get_all(connection_array *vector, uint32_t maxCount, CONNECTION **OUT_connectionArray);

CONNECTION* connection_array_get(connection_array *vector, uint32_t index);
int service_connection_array_get_all_connections(service_connection_array *vector, uint32_t maxCount, CONNECTION **OUT_connectionArray);
CONNECTION* service_connection_array_get_connection(service_connection_array *vector, uint32_t index);
CONNECTION* service_connection_array_get_aux(service_connection_array *vector, uint32_t indexService, uint32_t indexAux);
