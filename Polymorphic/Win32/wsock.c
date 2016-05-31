
// Network handling for WANGBLOWS

#define HAVE_STRUCT_TIMESPEC


#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#include <commands.h>
#include <vector.h>

#define MODE_SERVICE 0
#define MODE_PEER 1

#define PROTO_HTTP 133
#define PROTO_SMTP 134

#pragma comment(lib, "Ws2_32.lib")

WSADATA wsaData;
SOCKET acceptSocket = INVALID_SOCKET;

typedef struct {
	DWORD threadID;
	HANDLE threadHandle;
} THREAD_INFO;

// Complicated struct. Houses connection state and various data structures required for IOCP to function.
typedef struct {
	SOCKET socket; // the actual socket handle itself. the star of the show

	CONNECTION_INFO info;

	int protocol;
	int byteCount; // IOCP will put bytes buffered by response here. Although the wait function also returns the same thing. Basically used as a dummy for the IOCP create fucntion.
	int flags; // used by IOCP
	HANDLE checkAliveTimer; // the timer that periodically checks to see if the connection is still alive
	WSABUF buffer; // buffer where IOCP data is stored before completion packet is passed
	WSAOVERLAPPED overlap; // used by IOCP
} CONNECTION;

typedef struct {
	int size;      // slots used so far
	int capacity;  // total available slots
	CONNECTION **data;     // array of integers we're storing
} connection_vector;

typedef struct {
	connection_vector connections;
	string_vector strings;
	int_vector keys;
	int_vector vacancies;
} connection_array;

connection_array serviceConnections; // stores service connections by ID
connection_array peerConnecitons; // stores peer connections by ID

CONNECTION controlConnection;

THREAD_INFO listenThread;
THREAD_INFO acceptThread;
HANDLE completionPort;
HANDLE checkAliveTimerQueue;
const int numWorkerThreads = 30;
const maxConnections = 500;
//const checkAliveInterval = 300000;
const checkAliveInterval = 10000;
int numConnections = 0;
int isShutdown = 0;

//---------------------------------------------------------------------------------------------------------------------------------------------
//CONNECTION self-filling array

void connection_array_init(connection_array *vector) {
	// initialize size and capacity
	vector->connections.size = 0;
	vector->connections.capacity = VECTOR_INITIAL_CAPACITY;
	vector->strings.size = 0;
	vector->strings.capacity = VECTOR_INITIAL_CAPACITY;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;

	// allocate memory for vector->data
	vector->connections.data = malloc(sizeof(CONNECTION*) * vector->connections.capacity);
	vector->strings.data = malloc(sizeof(char*) * vector->strings.capacity);
	vector->keys.data = malloc(sizeof(int) * vector->strings.capacity);
	vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
}

void connection_array_init_capacity(connection_array *vector, int capacity) {
	// initialize size and capacity
	// initialize size and capacity
	vector->connections.size = 0;
	vector->connections.capacity = capacity;
	vector->strings.size = 0;
	vector->strings.capacity = capacity;
	vector->keys.size = 0;
	vector->keys.capacity = capacity;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = capacity;

	// allocate memory for vector->data
	vector->connections.data = malloc(sizeof(CONNECTION*) * vector->connections.capacity);
	vector->strings.data = malloc(sizeof(char*) * vector->strings.capacity);
	vector->keys.data = malloc(sizeof(int) * vector->strings.capacity);
	vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
}

void connection_array_double_capacity_if_full(connection_array *vector) {
	if (vector->connections.capacity == 0)
	{
		vector->connections.capacity = VECTOR_INITIAL_CAPACITY;
		vector->connections.data = malloc(sizeof(CONNECTION*) * vector->connections.capacity);
		vector->strings.capacity = VECTOR_INITIAL_CAPACITY;
		vector->strings.data = malloc(sizeof(char*) * vector->strings.capacity);
		vector->keys.capacity = VECTOR_INITIAL_CAPACITY;
		vector->keys.data = malloc(sizeof(int) * vector->strings.capacity);
	}
	if (vector->connections.size >= vector->connections.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->connections.capacity *= 2;
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION*) * vector->connections.capacity);
		vector->strings.capacity *= 2;
		vector->strings.data = realloc(vector->strings.data, sizeof(char*) * vector->strings.capacity);
		vector->keys.capacity *= 2;
		vector->keys.data = realloc(vector->strings.data, sizeof(char*) * vector->strings.capacity);
	}
}

void conenction_array_double_vacancy_capacity_if_full(connection_array *vector) {
	if (vector->vacancies.capacity == 0)
	{
		vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;
		vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
	}
	if (vector->vacancies.size >= vector->vacancies.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->vacancies.capacity *= 2;
		vector->vacancies.data = realloc(vector->vacancies.data, sizeof(int) * vector->vacancies.capacity);
	}
}

int connection_array_append(connection_array *vector, CONNECTION *connection, char *string, int key)
{
	if (connection == NULL || string == NULL || key == INT_MIN)
		return -1;

	// make sure there's room to expand into
	connection_array_double_capacity_if_full(vector);

	// append the value and increment vector->size
	int index = vector->connections.size++;
	vector->connections.data[index] = connection;
	vector->strings.data[vector->strings.size++] = string;
	vector->keys.data[vector->keys.size++] = key;
	return index;
}

int connection_array_get_vacancy(connection_array *vector)
{
	return vector->vacancies.data[vector->vacancies.size - 1];
}

void connection_array_pop_vacancy(connection_array *vector)
{
	vector->vacancies.data[vector->vacancies.size - 1] = INT_MIN;
	vector->vacancies.size--;
}

int connection_array_push(connection_array *vector, CONNECTION *connection, char *string, int key) {

	if (connection == NULL || string == NULL || key == INT_MIN)
		return -1;

	if (vector->vacancies.size > 0)
	{
		int index = connection_array_get_vacancy(vector);
		vector->connections.data[index] = connection;
		vector->strings.data[index] = string;
		vector->keys.data[index] = key;
		connection_array_pop_vacancy(vector);
		return index;
	}
	else
	{
		return connection_array_append(vector, connection, string, key);
	}

}

int connection_array_delete(connection_array *vector, int index)
{
	if (index >= vector->connections.size || index < 0 || vector->connections.data[index] == NULL)
		return -1;

	conenction_array_double_vacancy_capacity_if_full(vector);
	vector->vacancies.data[vector->vacancies.size++] = index;
	vector->connections.data[index] = NULL;
	vector->strings.data[index] = NULL;
	vector->keys.data[index] = INT_MIN;

	return 0;
}

CONNECTION* connection_array_get_connection(connection_array *vector, int index) {
	if (index >= vector->connections.size || index < 0)
		return NULL;

	return vector->connections.data[index];
}

char* connection_array_get_string(connection_array *vector, int index) {
	if (index >= vector->strings.size || index < 0)
		return NULL;

	return vector->strings.data[index];
}

int connection_array_get_key(connection_array *vector, int index) {
	if (index >= vector->strings.size || index < 0)
		return INT_MIN;

	return vector->keys.data[index];
}

int connection_array_set(connection_array *vector, int index, CONNECTION *connection, char *string, int key) {
	// fail if out of bounds

	if (connection == NULL || string == NULL || key == INT_MIN)
		return -1;

	if (vector->connections.data[index] == NULL)
		return -1;

	if (index >= vector->connections.size || index < 0) {
		return -1;
	}

	// set the value at the desired index
	vector->connections.data[index] = connection;
	vector->strings.data[index] = string;
	vector->keys.data[index] = key;
	return 0;
}

void connection_array_trim(connection_array *vector) {
	//trim capacity down to the size of the array
	if (vector->connections.size == 0)
	{
		//if there are no elements, free the memory and create a null pointer
		free(vector->connections.data);
		free(vector->strings.data);
		free(vector->keys.data);
		vector->connections.data = NULL;
		vector->strings.data = NULL;
		vector->keys.data = NULL;
		vector->connections.capacity = 0;
		vector->strings.capacity = 0;
		vector->keys.capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION*) * vector->connections.size);
		vector->strings.data = realloc(vector->strings.data, sizeof(char*) * vector->strings.size);
		vector->strings.data = realloc(vector->strings.data, sizeof(int) * vector->strings.size);
		vector->connections.capacity = vector->connections.size;
		vector->strings.capacity = vector->strings.size;
		vector->keys.capacity = vector->keys.size;
	}

	if (vector->vacancies.size == 0)
	{
		//if there are no elements, free the memory and create a null pointer
		free(vector->vacancies.data);
		vector->vacancies.data = NULL;
		vector->vacancies.capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->vacancies.data = realloc(vector->vacancies.data, sizeof(CONNECTION**) * vector->vacancies.size);
		vector->vacancies.capacity = vector->vacancies.size;
	}

}

void connection_array_free(connection_array *vector) {
	free(vector->connections.data);
	free(vector->strings.data);
	free(vector->keys.data);
	free(vector->vacancies.data);
}

//
//---------------------------------------------------------------------------------------------------------------------------------------------



int initSocketLib() 
{
	// Initialize Winsock
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int closeSocketLib()
{
	isShutdown = 1;
	return WSACleanup();
}

void closeConnection(CONNECTION *connection)
{
	shutdown(connection->socket, SD_BOTH);
	closesocket(connection->socket);
	if(connection->protocol == IPPROTO_TCP)
		DeleteTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, INVALID_HANDLE_VALUE);
	free(connection->buffer.buf);
	free(connection);
}


// Function that all worker threads will iterate
DWORD WINAPI workConnections(LPVOID dummy)
{

	while (1) {
		int bytesTansferred;
		CONNECTION *connection;
		OVERLAPPED *lap;

		GetQueuedCompletionStatus(completionPort, &bytesTansferred, (PULONG_PTR)&connection, &lap, INFINITE);

		if (isShutdown)
			break;

		printf("STARTING.\n");

		ChangeTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, checkAliveInterval, checkAliveInterval);

		// need to disable IOCP notifications

		if(connection->protocol == IPPROTO_TCP)
			processCommand((void*)connection->socket, connection->buffer.buf, connection->info);

	}
	ExitThread(0);
}


// WARN: We may need to synchronize this with sends on worker threads. Maybe not because send is blocking. I'm not sure.
// I imagine collisions will be very rare. If not synchronizing this causes a bug, it will be as rare.
VOID CALLBACK checkAlive(PVOID connection, BOOLEAN fired)
{
	printf("CHECK-ALIVE: %i\n", send(((CONNECTION*)connection)->socket, "still there?\n", 13, 0));

	// Send one ping command
}

// Function for control thread that accepts new connections
DWORD WINAPI acceptNewTCPConnections(LPVOID dummy)
{
	while (1)
	{
		CONNECTION *connection = malloc(sizeof(CONNECTION));
		connection->socket = accept(acceptSocket, NULL, NULL);
		if (isShutdown)
			break;
		if (numConnections == maxConnections)
		{
			send(connection->socket, "fuck off we're full\n", 21, 0);
			closeConnection(connection);
		}
		else
		{
			connection->buffer.len = 2;
			connection->buffer.buf = malloc(sizeof(char) * 2);

			char* buffer[4];

			if (4 != recv(connection->socket, buffer[0], 4, MSG_WAITALL))
			{
				printf("CONNECTION FAILED.\n");
				continue;
			}

			short realm = ntohs((((short)buffer[0]) << 8) | (short)buffer[1]);

			if (realm == 0x00)
			{
				connection->info.mode = MODE_SERVICE;
				short serviceStringSize = ntohs((((short)buffer[2]) << 8) | (short)buffer[3]);
				char* serviceString = malloc(sizeof(char) * serviceStringSize);
				
			}
			else
				connection->info.mode = MODE_PEER;

			connection->overlap.hEvent = NULL;
			connection->byteCount = 0;
			connection->flags = MSG_WAITALL;
			connection->protocol = IPPROTO_TCP;
			CreateTimerQueueTimer(&connection->checkAliveTimer, checkAliveTimerQueue, checkAlive, connection, checkAliveInterval, checkAliveInterval, 0);
			CreateIoCompletionPort((HANDLE)connection->socket, completionPort, (ULONG_PTR)connection, 0);
			WSARecv(connection->socket, &connection->buffer, 1, &connection->byteCount, &connection->flags, &connection->overlap, NULL);
			printf("CONNECTIONS: %i\n", ++numConnections);
		}
	}
	ExitThread(0);
}

int startListenSocket(char* port) 
{
	initSocketLib();
	checkAliveTimerQueue = CreateTimerQueue();

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	connection_array_init(&serviceConnections);
	connection_array_init(&peerConnecitons);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	int iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to create the socket
	acceptSocket = INVALID_SOCKET;
	acceptSocket = WSASocketW(result->ai_family, result->ai_socktype, result->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (acceptSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 0;
	}

	// Setup the TCP listening socket
	iResult = bind(acceptSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(acceptSocket);
		WSACleanup();
		return 0;
	}

	freeaddrinfo(result);

	// Begin to listen on socket
	if (listen(acceptSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numWorkerThreads);

	for (int x = 0; x < numWorkerThreads; x++)
	{
		if (
			CreateThread(
				NULL,
				0,
				workConnections,
				NULL,
				0,
				NULL)
			== NULL)
		{
			printf("Create thread failed: %d\n", WSAGetLastError());
			return 0;
		}
	}

	acceptThread.threadHandle = CreateThread(
		NULL,
		0,
		acceptNewTCPConnections,
		NULL,
		0,
		&acceptThread.threadID
	);

	return 1;
}