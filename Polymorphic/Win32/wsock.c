#define HAVE_STRUCT_TIMESPEC
#define OPENSSL_SYS_WINDOWS

#define POLY_PLATFORM_WANGBLOWS

#define MAX_CONNECTIONS 500

#include <definitions.h>
#include <stdio.h>

#include <Win32/wsock_datatypes.h>

#pragma comment(lib, "Ws2_32.lib")

WSADATA wsaData;
SOCKET acceptSocket = INVALID_SOCKET;

//multi-threaded vaiables
connection_array peerConnections; // stores peer connections by ID
HANDLE peerConnectionsCriticalSection; // sync object
service_connection_array serviceConnections; // stores main service connections by ID
HANDLE serviceConnectionsCriticalSection; // sync object
char serviceString[65536]; // stores the services string for this peer
HANDLE serviceStringCriticalSection; // sync object
int numWorkerThreads = 0; // number of worker threads currently active
HANDLE numWorkerThreadsCriticalSection; // sync object

//non-threaded variables
THREAD_INFO acceptThread; // thread that accepts incoming connections
HANDLE completionPort; // stores all connections that are currently waiting on events
HANDLE checkAliveTimerQueue; // timer queue where all the check alive timers are stored
const uint32_t maxWorkerThreads = 30; // number of worker threads to spawn to work connection events
const uint32_t maxConnections = MAX_CONNECTIONS; // maximum number of connections this peer will allow
const uint32_t checkAliveInterval = 300000; // the rate at which this peer will check to see if idle connections are still reachable
const uint32_t newConnectionTimeoutInterval = 5000; // the amount of time this peer will wait on a response to a greeting before dropping the connection
uint32_t numConnections = 0; // the number of current connections
uint8_t isShutdown = 0; // shutdown state of the daemon

POLYM_OVERLAPPED shutdownOverlap; //overlap for the shutdown function

int initSocketLib() 
{
	// Initialize Winsock
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
	shutdownOverlap.eventType = POLYM_EVENT_SHUTDOWN;
}

int closeSocketLib()
{
	isShutdown = 1;
	return WSACleanup();
}

void insertShortIntoBuffer(uint8_t *buffer, uint16_t unconvertedShort)
{
	uint16_t convertedShort = htons(unconvertedShort);
	buffer[0] = convertedShort >> 8;
	buffer[1] = convertedShort & 0x00FF;
}

void insertShortIntoBufferNC(uint8_t *buffer, uint16_t unconvertedShort)
{
	buffer[0] = unconvertedShort >> 8;
	buffer[1] = unconvertedShort & 0x00FF;
}

void insertLongIntoBuffer(uint8_t *buffer, uint16_t unconvertedLong)
{
	uint16_t convertedLong = htons(unconvertedLong);
	buffer[0] = convertedLong >> 24;
	buffer[1] = (convertedLong >> 16) & 0x000000FF;
	buffer[2] = (convertedLong >> 8) & 0x000000FF;
	buffer[3] = convertedLong & 0x000000FF;
}

void insertLongIntoBufferNC(uint8_t *buffer, uint16_t unconvertedLong)
{
	buffer[0] = unconvertedLong >> 24;
	buffer[1] = (unconvertedLong >> 16) & 0x000000FF;
	buffer[2] = (unconvertedLong >> 8) & 0x000000FF;
	buffer[3] = unconvertedLong & 0x000000FF;
}

uint16_t getShortFromBuffer(uint8_t* buffer)
{
	return ntohs((((uint16_t)buffer[0]) << 8) | buffer[1]);
}

uint16_t getShortFromBufferNC(uint8_t* buffer)
{
	return (((uint16_t)buffer[0]) << 8) | buffer[1];
}

uint32_t getLongFromBuffer(uint8_t* buffer)
{
	return ntohs((((uint32_t)buffer[0]) << 24) | buffer[1] << 16 | buffer[2] << 8 | buffer[3]);
}

uint32_t getLongFromBufferNC(uint8_t* buffer)
{
	return (((uint32_t)buffer[0]) << 24) | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

void initializeOverlap(POLYM_OVERLAPPED* overlap)
{
	overlap->hEvent = 0;
	overlap->Internal = 0;
	overlap->InternalHigh = 0;
	overlap->Offset = 0;
	overlap->OffsetHigh = 0;
	overlap->Pointer = 0;
}

// attempt to close a connection gracefully.
int closeConnection(CONNECTION *connection)
{
	shutdown(connection->socket, SD_BOTH);
	closesocket(connection->socket);
	switch (connection->info.protocol)
	{
	case POLYM_PROTO_TCP:
		DeleteTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, INVALID_HANDLE_VALUE);
		break;
	}
	switch (connection->info.mode)
	{
	case POLYM_MODE_SERVICE:
		free(connection->info.mode_info.service.serviceString);
		break;
	}
	free(connection->buffer.buf);
	free(connection);

	return 0; // this will return 0 upon graceful shutdown, error code if disconnect command fails.
}

int closeUnitializedConnection(void *connection)
{
	return closeConnection((CONNECTION*)connection);
}

VOID CALLBACK checkAlive(PVOID connection, BOOLEAN fired)
{
	printf("CHECK-ALIVE: %i\n", send(((CONNECTION *)connection)->socket, "still there?\n", 13, 0));
	// Send one ping command
}

VOID CALLBACK newConnectionTimeout(PVOID connection, BOOLEAN fired)
{
	closeConnection((CONNECTION *)connection);
}

//TCP send. TODO: Standardize error codes and flags.
int tcpSend(SOCKET socket, uint8_t *buffer, uint32_t length, int32_t flags)
{
	return send(socket, buffer, length, flags);
}


int tcpRecv(SOCKET socket, uint8_t *buffer, uint32_t length, int32_t flags)
{
	return recv(socket, buffer, length, flags);
}

int sockSend(void* connection, uint8_t *buffer, uint32_t length)
{
	CONNECTION* connPointer = ((CONNECTION*)connection);
	switch (connPointer->info.protocol)
	{
	case POLYM_PROTO_TCP:
		tcpSend(connPointer->socket, buffer, length, 0);
		break;
	}
}

int tcpSendAsync(SOCKET socket, WSABUF *wsabuffer, int32_t flags)
{
	POLYM_OVERLAPPED *overlap = malloc(sizeof(POLYM_OVERLAPPED));
	overlap->eventType = POLYM_EVENT_ASYNC_SEND;
	overlap->eventInfo = wsabuffer;
	initializeOverlap(overlap);
	return WSASend((SOCKET)socket, wsabuffer, 1, NULL, 0, overlap, NULL);
}

int sockSendAsync(void* connection, uint8_t *buffer, uint32_t length)
{
	CONNECTION* connPointer = ((CONNECTION*)connection);

	WSABUF *wsabuffer = malloc(sizeof(WSABUF));
	wsabuffer->buf = buffer;
	wsabuffer->len = length;

	switch (connPointer->info.protocol)
	{
	case POLYM_PROTO_TCP:
		tcpSendAsync(connPointer->socket, wsabuffer, 0);
		break;
	}
}

int sockRecv(void* connection, uint8_t *buffer, uint32_t length)
{
	CONNECTION* connPointer = ((CONNECTION*)connection);

	switch (connPointer->info.protocol)
	{
	case POLYM_PROTO_TCP:
		tcpRecv(connPointer->socket, buffer, length, MSG_WAITALL);
		break;
	}
}

char* intIPtoStringIP(uint32_t ipv4AddressLong, char *OUT_StringIP, int bufferSize)
{
	struct in_addr addressOjbect;
	addressOjbect.S_un.S_addr = ipv4AddressLong;
	return inet_ntop(AF_INET, &addressOjbect, OUT_StringIP, bufferSize);
}

void lockConnectionMutex(CONNECTION *connection)
{
	EnterCriticalSection(&connection->connectionMutex);
}

void lockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info)
{
	switch (info->mode)
	{
	case POLYM_MODE_SERVICE:
		lockConnectionMutex(service_connection_array_get_connection(&serviceConnections, info->mode_info.service.serviceID));
		return;
	case POLYM_MODE_SERVICE_AUX:
		lockConnectionMutex(service_connection_array_get_aux(&serviceConnections, info->mode_info.serviceAux.serviceID, info->mode_info.serviceAux.servicePort));
		return;
	case POLYM_MODE_PEER:
		lockConnectionMutex(connection_array_get(&peerConnections, info->mode_info.peer.peerID));
		return;
	default:
		return;
	}
}

void unlockConnectionMutex(CONNECTION *connection)
{
	LeaveCriticalSection(&connection->connectionMutex);
}


void unlockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info)
{
	switch (info->mode)
	{
	case POLYM_MODE_SERVICE:
		unlockConnectionMutex(service_connection_array_get_connection(&serviceConnections, info->mode_info.service.serviceID));
		return;
	case POLYM_MODE_SERVICE_AUX:
		unlockConnectionMutex(service_connection_array_get_aux(&serviceConnections, info->mode_info.serviceAux.serviceID, info->mode_info.serviceAux.servicePort));
		return;
	case POLYM_MODE_PEER:
		unlockConnectionMutex(connection_array_get(&peerConnections, info->mode_info.peer.peerID));
		return;
	default:
		return;
	}
}

// function that all event listener threads will execute
DWORD WINAPI eventListener(LPVOID dummy)
{

	while (1) {
		// return values for GetQueuedCompletionStatus
		uint32_t bytesTansferred;
		CONNECTION *connection;
		POLYM_OVERLAPPED *overlap;

		// wait to recieve an event
		GetQueuedCompletionStatus(completionPort, &bytesTansferred, (PULONG_PTR)&connection, &overlap, INFINITE);

		switch (overlap->eventType)
		{

		case POLYM_EVENT_LISTEN:
			// reset the check alive timer because of this activity
			ChangeTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, checkAliveInterval, checkAliveInterval);

			// if nothing was recieved, the connection was closed. dispose of it.
			if (bytesTansferred == 0)
				closeConnection(connection);

			// process the command
			lockConnectionMutex(connection);
			processCommand((void*)connection, connection->buffer.buf, &connection->info);
			unlockConnectionMutex(connection);

			//rearm the connection on the listen list
			WSARecv(connection->socket, &connection->buffer, 1, &connection->byteCount, &connection->flags, &connection->overlap, NULL);

		case POLYM_EVENT_ASYNC_SEND:
			// reset the check alive timer because of this activity
			ChangeTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, checkAliveInterval, checkAliveInterval);

			free(((WSABUF*)overlap->eventInfo)->buf); // free the buffer
			free((WSABUF*)overlap->eventInfo); // free the WSABUF object
			free(overlap); // free the OVERLAPPED object
			continue;

		case POLYM_EVENT_SHUTDOWN:
			EnterCriticalSection(numWorkerThreadsCriticalSection);
			numWorkerThreads--;
			LeaveCriticalSection(numWorkerThreadsCriticalSection);
			ExitThread(0);
			return 0;

		default:
			break; // TODO: critical error log and exit
		}
	}
}

int closeWorkerThreads(int count)
{
	int countReturn = 0;
	for (int x = 0; x < count; x++)
	{
		countReturn += PostQueuedCompletionStatus(completionPort, 0, NULL, &shutdownOverlap);
	}
	return countReturn;
}

POLYM_CONNECTION_INFO* getServiceConnectionInfo(int index)
{
	EnterCriticalSection(serviceConnectionsCriticalSection);
	POLYM_CONNECTION_INFO *ret = &service_connection_array_get_connection(&serviceConnections, index)->info;
	LeaveCriticalSection(serviceConnectionsCriticalSection);
	return ret;
}

int serviceStringExists(char* string) 
{
	EnterCriticalSection(serviceConnectionsCriticalSection);
	int ret = service_connection_array_service_string_exists(&serviceConnections, string);
	LeaveCriticalSection(serviceConnectionsCriticalSection);
	return ret;
}

int addNewService(void* connection)
{
	// TODO: need to fail if we have USHORT_MAX services without any vacancies. needs to return either negative error code or a value within the bounds of a USHORT.
	EnterCriticalSection(serviceStringCriticalSection);
	strcat(serviceString, ((CONNECTION*)connection)->info.mode_info.service.serviceString);
	strcat(serviceString, "|");
	LeaveCriticalSection(serviceStringCriticalSection);
	EnterCriticalSection(serviceConnectionsCriticalSection);
	int ret = service_connection_array_push(&serviceConnections, (CONNECTION*)connection);
	LeaveCriticalSection(serviceConnectionsCriticalSection);
	return ret;
}

int32_t removeService(uint16_t serviceID)
{
	EnterCriticalSection(serviceConnectionsCriticalSection);
	CONNECTION* connection = service_connection_array_get_connection(&serviceConnections, serviceID);

	if (connection = NULL)
		return POLYM_ERROR_SERVICE_DOES_NOT_EXIST;

	int result = service_connection_array_delete(&serviceConnections, serviceID);
	LeaveCriticalSection(serviceConnectionsCriticalSection);
	
	if (result == 0)
		return closeConnection(connection);
	else
		return result;

	rebuildServiceString();
}

int addNewServiceAux(uint16_t serviceID, void* connection)
{
	EnterCriticalSection(serviceConnectionsCriticalSection);
	int ret = service_connection_array_push_aux(&serviceConnections, serviceID, (CONNECTION*)connection);
	LeaveCriticalSection(serviceConnectionsCriticalSection);
	return ret;
}

int32_t removeServiceAux(uint16_t serviceID, uint16_t serviceAuxID)
{
	EnterCriticalSection(serviceConnectionsCriticalSection);
	CONNECTION* connection = service_connection_array_get_aux(&serviceConnections, serviceID, serviceAuxID);

	if (connection = NULL)
		return POLYM_ERROR_SERVICE_DOES_NOT_EXIST;

	int result = service_connection_array_delete_aux(&serviceConnections, serviceID, serviceAuxID);
	LeaveCriticalSection(serviceConnectionsCriticalSection);

	if (result == 0)
		return closeConnection(connection);
	else
		return result;
}

int addNewPeer(void* connection)
{
	EnterCriticalSection(peerConnectionsCriticalSection);
	int ret = connection_array_push(&peerConnections, (CONNECTION*)connection);
	LeaveCriticalSection(peerConnectionsCriticalSection);
	return ret;
}

int32_t removePeer(uint16_t peerID)
{
	EnterCriticalSection(peerConnectionsCriticalSection);
	CONNECTION* connection = connection_array_get(&peerConnections, peerID);

	if (connection = NULL)
		return POLYM_ERROR_SERVICE_DOES_NOT_EXIST;

	int result = connection_array_delete(&peerConnections, peerID);
	LeaveCriticalSection(peerConnectionsCriticalSection);

	if (result == 0)
		return closeConnection(connection);
	else
		return result;
}

void closeAllServiceAux(uint16_t serviceID)
{
	EnterCriticalSection(serviceConnectionsCriticalSection);
	service_connection_array_close_all_aux(&serviceConnections, serviceID);
	LeaveCriticalSection(serviceConnectionsCriticalSection);
}

void* getConnectionFromPeerID(uint16_t peerID)
{
	EnterCriticalSection(peerConnectionsCriticalSection);
	void *ret = connection_array_get(&peerConnections, peerID);
	LeaveCriticalSection(peerConnectionsCriticalSection);
	return ret;
}

void* getConnectionFromServiceID(uint16_t serviceID, uint16_t portID)
{
	if (portID == 0)
	{
		EnterCriticalSection(serviceConnectionsCriticalSection);
		void *ret = service_connection_array_get_connection(&serviceConnections, serviceID);
		LeaveCriticalSection(serviceConnectionsCriticalSection);
		return ret;
	}
	else
	{
		EnterCriticalSection(serviceConnectionsCriticalSection);
		void *ret = service_connection_array_get_aux(&serviceConnections, serviceID, portID);
		LeaveCriticalSection(serviceConnectionsCriticalSection);
		return ret;
	}
}

POLYM_CONNECTION_INFO* getInfoFromConnection(void *connection)
{
	return &((CONNECTION*)connection)->info;
}

int getCurrentServiceConnections(void** OUT_connectionArray, uint32_t maxCount)
{
	EnterCriticalSection(serviceConnectionsCriticalSection);
	int ret = service_connection_array_get_all_connections(&serviceConnections, maxCount, OUT_connectionArray);
	LeaveCriticalSection(serviceConnectionsCriticalSection);
	return ret;
}

int getCurrentPeerConnections(void** OUT_connectionArray, uint32_t maxCount)
{
	EnterCriticalSection(peerConnectionsCriticalSection);
	int ret = connection_array_get_all(&peerConnections, maxCount, OUT_connectionArray);
	LeaveCriticalSection(peerConnectionsCriticalSection);
	return ret;
}

int getPeerConnectionAddressString(int peerID, int stringBufferSize, char* OUT_stringBuffer)
{
	EnterCriticalSection(peerConnectionsCriticalSection);
	CONNECTION* connection = connection_array_get(&peerConnections, peerID);
	LeaveCriticalSection(peerConnectionsCriticalSection);
	switch (connection->info.addrtype)
	{
	case IPPROTO_IPV4:
		inet_ntop(AF_INET, &connection->address.ipv4.sin_addr, OUT_stringBuffer, stringBufferSize);
	case IPPROTO_IPV6:
		return -1; // ipv6 unimplemented
	default:
		return -1; // TODO: this should never happen, but throw an error just in case
	}
}

int compareAddresses(struct in_addr address1, struct in_addr address2)
{
	return address1.S_un.S_addr - address2.S_un.S_addr;
}

int addressConnected(char *stringAddress, uint16_t port)
{
	struct in_addr address1;
	inet_pton(AF_INET, stringAddress, &address1);
	CONNECTION* connections[MAX_CONNECTIONS];
	int numConnections = getCurrentPeerConnections(&connections, MAX_CONNECTIONS);
	int returnValue = 0;

	for (int x = 0; x < numConnections; x++)
	{
		if (0 != compareAddresses(address1, connections[x]->address.ipv4.sin_addr))
		{
			returnValue = 1;
			if (port == connections[x]->address.ipv4.sin_port)
				return 2;
		}
	}
	return returnValue;
}

int rebuildServiceString()
{
	CONNECTION *services[65536];
	int connectionCount = getCurrentServiceConnections(&services, 65536);
	EnterCriticalSection(serviceStringCriticalSection);
	strcpy(serviceString, "");
	for (int x = 0; x < connectionCount; x++) 
	{
		strcat(serviceString, services[x]->info.mode_info.service.serviceString);
		strcat(serviceString, "|");
	}
	LeaveCriticalSection(serviceConnectionsCriticalSection);
}

void initializeNewTCPConnection(CONNECTION *connection)
{
	const DWORD sock_timeout = 5 * 1000;
	setsockopt(connection->socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));
	setsockopt(connection->socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));
	connection->overlap.eventInfo = NULL;
	connection->overlap.eventType = 0;
	initializeOverlap(&connection->overlap);
	connection->buffer.len = 2;
	connection->buffer.buf = malloc(sizeof(uint8_t) * 2);
	connection->overlap.hEvent = NULL;
	connection->byteCount = 0;
	connection->flags = MSG_WAITALL;
	connection->info.mode = POLYM_MODE_UNINIT;
	connection->info.protocol = POLYM_PROTO_TCP;
	connection->info.addrtype = IPPROTO_IPV4;
	InitializeCriticalSection(&connection->connectionMutex);
}

void* openNewTCPConnection(char *ipAddress, char *l4Port, POLYM_CONNECTION_INFO **out_connectionInfo)
{
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	int iResult = getaddrinfo(ipAddress, l4Port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return NULL;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;

	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr = result;

	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
		ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return NULL;
	}

	CONNECTION *connection = malloc(sizeof(CONNECTION));
	// Connect to server.
	connection->socket = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (connection->socket == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	// Should really try the next address returned by getaddrinfo
	// if the connect call failed
	// But for this simple example we just free the resources
	// returned by getaddrinfo and print an error message

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return NULL;
	}

	initializeNewTCPConnection(connection);
	connection->info.mode = POLYM_MODE_PEER;

	out_connectionInfo = &connection->info;
	return connection;
}

void* openNewConnection(char *ipAddress, char *l4Port, POLYM_CONNECTION_INFO **out_connectionInfo, uint8_t protocol)
{
	switch (protocol)
	{
	case POLYM_PROTO_TCP:
		return openNewTCPConnection(ipAddress, l4Port, out_connectionInfo);
	}
	return NULL;
}

// function for the control thread that accepts new connections
DWORD WINAPI acceptNewTCPConnections(LPVOID dummy)
{
	while (1)
	{

		// create a new empty connection object and wait for new connections
		CONNECTION *connection = malloc(sizeof(CONNECTION));
		connection->socket = accept(acceptSocket, &connection->address.ipv4, (int)sizeof(struct sockaddr_in));
		

		// don't proceed if shutting down. accept will unblock but no connection will be present. break instead.
		if (isShutdown)
			break;

		// refuse connections if we're already at max.
		if (numConnections == maxConnections)
		{
			// send error code for full
			send(connection->socket, htons((uint16_t)POLYM_ERROR_MAX_CONNECTIONS), 2, 0);
			shutdown(connection->socket, SD_BOTH);
			closesocket(connection->socket);
			free(connection);
		}
		else
		{
			//set up the socket and put it on the IOCP listening queue
			printf("CONNECTIONS: %i\n", ++numConnections);
			initializeNewTCPConnection(connection);
			CreateTimerQueueTimer(&connection->checkAliveTimer, checkAliveTimerQueue, newConnectionTimeout, connection, newConnectionTimeoutInterval, newConnectionTimeoutInterval, 0);
			CreateIoCompletionPort((HANDLE)connection->socket, completionPort, (ULONG_PTR)connection, 0);

			// OPTI: initializing connections may need to become multithreaded.
			if (POLYM_MODE_FAILED == initializeIncomingConnection(connection->socket, connection, connection->info))
			{
				printf("CONNECTION FAILED.\n");
				closeConnection(connection);
			}
			else
				WSARecv(connection->socket, &connection->buffer, 1, &connection->byteCount, &connection->flags, &connection->overlap, NULL);
		}
	}
	ExitThread(0);
	return 0;
}

int startListenSocket(char* port) 
{
	initSocketLib();
	checkAliveTimerQueue = CreateTimerQueue();

	InitializeCriticalSection(peerConnectionsCriticalSection);
	InitializeCriticalSection(serviceConnectionsCriticalSection);
	InitializeCriticalSection(serviceStringCriticalSection);
	InitializeCriticalSection(numWorkerThreadsCriticalSection);

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	RAND_poll();

	connection_array_init(&serviceConnections);
	connection_array_init(&peerConnections);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	// Resolve the local address and port to be used by the server
	uint32_t iResult = getaddrinfo(NULL, port, &hints, &result);
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
	iResult = bind(acceptSocket, result->ai_addr, (uint32_t)result->ai_addrlen);
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

	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, maxWorkerThreads);

	EnterCriticalSection(numWorkerThreadsCriticalSection);
	for (numWorkerThreads = 0; numWorkerThreads < maxWorkerThreads; numWorkerThreads++)
	{
		if (
			CreateThread(
				NULL,
				0,
				eventListener,
				NULL,
				0,
				NULL) == NULL)
		{
			printf("Create thread failed: %d\n", WSAGetLastError());
			return 0;
		}
	}
	LeaveCriticalSection(numWorkerThreadsCriticalSection);

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