#pragma once

#ifndef WSOCKH
#include <winsock2.h>
#define WSOCKH
#endif // !WSOCKH

#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <vector.h>
#include <stdint.h>
#include <info_structure.h>

typedef struct {
	DWORD threadID;
	HANDLE threadHandle;
} THREAD_INFO;

// Complicated struct. Houses connection state and various data structures required for IOCP to function.
typedef struct {
	SOCKET socket; // the actual socket handle itself. the star of the show

				   // connection information
	CONNECTION_INFO info; // info struct, containing info shared with the commands.c file. defnition in commands header file.
	uint8_t encryptionType; // kind of encryption key used for this connection
	uint8_t encryptionKey[32]; // the encryption key used to decrypt traffic
	HANDLE checkAliveTimer; // the timer that periodically checks to see if the connection is still alive

							// IOCP internals
	int32_t byteCount; // IOCP will put bytes buffered by response here. Although the wait function also returns the same thing. Basically used as a dummy for the IOCP create fucntion.
	int32_t flags; // used for IOCP
	WSABUF buffer; // buffer where IOCP data is stored before completion packet is passed
	WSAOVERLAPPED overlap; // used for IOCP

	union address {
		struct sockaddr_in ipv4;
		struct sockaddr_in6 ipv6;
	} address;
} CONNECTION;

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

CONNECTION* service_connection_array_get_connection(service_connection_array *vector, uint32_t index);

/*
void connection_array_init(connection_array *vector);
void connection_array_init_capacity(connection_array *vector, uint32_t capacity);
void connection_array_double_capacity_if_full(connection_array *vector);
void connection_array_double_vacancy_capacity_if_full(connection_array *vector);
int connection_array_append(connection_array *vector, CONNECTION *connection);
int connection_array_pop_vacancy(connection_array *vector);
int connection_array_push(connection_array *vector, CONNECTION *connection);
int connection_array_element_exists(connection_array *vector, uint32_t index);
int connection_array_delete(connection_array *vector, uint32_t index);
CONNECTION* connection_array_get(connection_array *vector, uint32_t index);
int connection_array_set(connection_array *vector, uint32_t index, CONNECTION *connection);
void connection_array_trim(connection_array *vector);
int connection_array_free(connection_array *vector);

void service_connection_array_init(service_connection_array *vector);
void service_connection_array_init_capacity(service_connection_array *vector, uint32_t capacity);
void service_connection_array_double_capacity_if_full(service_connection_array *vector);
void service_connection_array_double_vacancy_capacity_if_full(service_connection_array *vector);
int service_connection_array_append(service_connection_array *vector, CONNECTION *connection);
int service_connection_array_pop_vacancy(service_connection_array *vector);
*/
