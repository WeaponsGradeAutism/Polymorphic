#define HAVE_STRUCT_TIMESPEC
#define OPENSSL_SYS_WINDOWS

#define POLYM_PLATFORM_WANGBLOWS

#define MAX_CONNECTIONS 500

#include <stdio.h>

#include <Win32/wsock_datatypes.h>
#include <commands.h>
#include <connections.h>
#include <definitions.h>

#pragma comment(lib, "Ws2_32.lib")

WSADATA wsaData;
SOCKET acceptSocket = INVALID_SOCKET;

//multi-threaded vaiables
connection_array peerConnections; // stores peer connections by ID
CRITICAL_SECTION peerConnectionsCriticalSection; // sync object
connection_array serviceConnections; // stores service connections by ID
CRITICAL_SECTION serviceConnectionsCriticalSection; // sync object
connection_array clientConnections; // stores client connections by ID
CRITICAL_SECTION clientConnectionsCriticalSection; // sync object
message_buffer_array messageSpace; // stores main service connections by ID
CRITICAL_SECTION messageSpaceCriticalSection; // sync object
char serviceString[65536]; // stores the services string for this peer
CRITICAL_SECTION serviceStringCriticalSection; // sync object
int numWorkerThreads = 0; // number of worker threads currently active
CRITICAL_SECTION numWorkerThreadsCriticalSection; // sync object

//non-threaded variables
THREAD_INFO acceptThread; // thread that accepts incoming connections
HANDLE completionPort; // stores all connections that are currently waiting on events
HANDLE checkAliveTimerQueue; // timer queue where all the check alive timers are stored
const int maxWorkerThreads = 30; // number of worker threads to spawn to work connection events
const int maxConnections = MAX_CONNECTIONS; // maximum number of connections this peer will allow
const int checkAliveInterval = 300000; // the rate at which this peer will check to see if idle connections are still reachable
const int newConnectionTimeoutInterval = 5000; // the amount of time this peer will wait on a response to a greeting before dropping the connection
uint32_t numConnections = 0; // the number of current connections
uint8_t isShutdown = 0; // shutdown state of the daemon

POLYM_OVERLAPPED shutdownOverlap; //overlap for the shutdown function

///<summary> Initializes the winsock library. </summary>
int initSocketLib() 
{
	// Initialize Winsock
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
	shutdownOverlap.eventType = POLYM_EVENT_SHUTDOWN;
}

///<summary> Shuts down the Polymorphic server. </summary>
int closeListenSocket()
{
	isShutdown = 1;
	return WSACleanup();
}

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

///<summary> Initializes the typically unused POLYM_OVERLAPPED object values to defaults. <summary>
void initializeOverlap(POLYM_OVERLAPPED* overlap)
{
	overlap->hEvent = 0;
	overlap->Internal = 0;
	overlap->InternalHigh = 0;
	overlap->Offset = 0;
	overlap->OffsetHigh = 0;
	overlap->Pointer = 0;
}

///<summary> Attempt to close a connection gracefully. <summary>
int closeConnection(POLYM_CONNECTION *connection)
{
	shutdown(connection->socket, SD_BOTH);
	closesocket(connection->socket);
	switch (connection->protocol)
	{
	case POLY_PROTO_TCP:
		DeleteTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, INVALID_HANDLE_VALUE);
		break;
	}
	switch (connection->info.realm)
	{
	case POLY_REALM_SERVICE:
		free(connection->info.realm_info.service.serviceString);
		break;
	}
	free(connection->buffer.buf);
	free(connection);

	return 0; // this will return 0 upon graceful shutdown, error code if disconnect command fails.
}

///<summary> Attempt to close a connection that isn't on the connection list gracefully using a generic pointer. </summary>
int closeUnitializedConnection(void *connection)
{
	return closeConnection((POLYM_CONNECTION*)connection);
}

///<summary> Sends the check alive command. </summary>
VOID CALLBACK checkAlive(PVOID connection, BOOLEAN fired) //TODO: this needs to be changed to a regular command
{
	printf("CHECK-ALIVE: %i\n", send(((POLYM_CONNECTION *)connection)->socket, "still there?\n", 13, 0));
	// Send one ping command
}

///<summary> Executes when a new connection times out before it can be interacted with. </summary>
VOID CALLBACK newConnectionTimeout(PVOID connection, BOOLEAN fired)
{
	closeConnection((POLYM_CONNECTION *)connection);
}

// TODO: Standardize error codes and flags.
///<summary> Send command for TCP based connections. </summary>
int tcpSend(SOCKET socket, uint8_t *buffer, uint32_t length, int32_t flags)
{
	return send(socket, buffer, length, flags);
}

///<summary> Recv command for TCP based connections. </summary>
int tcpRecv(SOCKET socket, uint8_t *buffer, uint32_t length, int32_t flags)
{
	return recv(socket, buffer, length, flags);
}

///<summary> Send command for a socket, using its protocol's send command. </summary>
int sockSend(void* connection, uint8_t *buffer, uint32_t length)
{
	POLYM_CONNECTION* connPointer = ((POLYM_CONNECTION*)connection);
	switch (connPointer->protocol)
	{
	case POLY_PROTO_TCP:
		return tcpSend(connPointer->socket, buffer, length, 0);
		break;
	}
	return -1;
}

///<summary> Async send over the TCP protocol. </summary>
int tcpSendAsync(SOCKET socket, WSABUF *wsabuffer, int32_t flags, OVERLAPPED *overlap)
{
	
	return WSASend((SOCKET)socket, wsabuffer, 1, NULL, 0, (OVERLAPPED*)overlap, NULL);
}

///<summary> Perform an asyncronous send on the supplied socket. </summary>
int sockSendAsync(void* connection, POLYM_MESSAGE_BUFFER *buffer)
{

	message_buffer *message_buffer = buffer->containerObject;

	// copy buffer to space and set length
	message_buffer->wsabuf.buf = message_buffer->buffer_object.buf;
	message_buffer->wsabuf.len = message_buffer->buffer_object.messageSize;

	// create the overlapped info for the event 
	message_buffer->overlap.eventType = POLYM_EVENT_ASYNC_SEND;
	message_buffer->overlap.eventInfo = &message_buffer;
	initializeOverlap(&message_buffer->overlap);

	switch ( ((POLYM_CONNECTION*)connection)->protocol )
	{
	case POLY_PROTO_TCP:
		return tcpSendAsync( ((POLYM_CONNECTION*)connection)->socket, &message_buffer->wsabuf, 0, (OVERLAPPED*)&message_buffer->overlap );
		break;
	}
	return -1;
}

///<summary> Recv command for a socket, using its protocol's recv command. </summary>
int sockRecv(void* connection, uint8_t *buffer, uint32_t length)
{
	POLYM_CONNECTION* connPointer = ((POLYM_CONNECTION*)connection);

	switch (connPointer->protocol)
	{
	case POLY_PROTO_TCP:
		return tcpRecv(connPointer->socket, buffer, length, MSG_WAITALL);
		break;
	}
	return -1;
}

///<summary> Converts an integer ipv4 representation to its string representation. </summary>
const char* intIPtoStringIP(uint32_t ipv4AddressLong, char *out_StringIP, int bufferSize)
{
	struct in_addr addressOjbect;
	addressOjbect.S_un.S_addr = ipv4AddressLong;
	return inet_ntop(AF_INET, &addressOjbect, out_StringIP, bufferSize);
}

///<summary> Locks the synchronization object associated with a connection object. </summary>
void lockConnectionMutex(POLYM_CONNECTION *connection)
{
	EnterCriticalSection(connection->connectionMutex);
}

///<summary> Locks the synchronization object associated with a connection object, using its info object. </summary>
void lockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info)
{
	lockConnectionMutex(connection_array_get(&peerConnections, info->realm_info.peer.peerID));
}

///<summary> Unlocks the synchronization object associated with a connection object.</summary>
void unlockConnectionMutex(POLYM_CONNECTION *connection)
{
	LeaveCriticalSection(connection->connectionMutex);
}

///<summary> Unlocks the synchronization object associated with a connection object, using its info object. </summary>
void unlockConnectionMutexByInfo(POLYM_CONNECTION_INFO *info)
{
	unlockConnectionMutex(connection_array_get(&serviceConnections, info->realm_info.service.serviceID));
}


///<summary> This is the main event loop that worker threads will run. </summary>
DWORD WINAPI eventListener(LPVOID dummy)
{

	while (1) {
		// return values for GetQueuedCompletionStatus
		uint32_t bytesTansferred;
		POLYM_CONNECTION *connection;
		POLYM_OVERLAPPED *overlap;

		// wait to recieve an event
		GetQueuedCompletionStatus(completionPort, &bytesTansferred, (PULONG_PTR)&connection, (OVERLAPPED**)&overlap, INFINITE);

		switch (overlap->eventType)
		{

		case POLYM_EVENT_LISTEN: // Recieved data on one of the sockets that are listening.
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
			WSARecv(connection->socket, &connection->buffer, 1, &connection->byteCount, &connection->flags, (OVERLAPPED*)&connection->overlap, NULL);

		case POLYM_EVENT_ASYNC_SEND: // An asynchrounous send has completed
			// reset the check alive timer because of this activity
			ChangeTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, checkAliveInterval, checkAliveInterval);

			message_buffer_array_free(&messageSpace, ((message_buffer*)overlap->eventInfo)->index);
			continue;

		case POLYM_EVENT_SHUTDOWN: // This thread has recieved a shutdown event
			EnterCriticalSection(&numWorkerThreadsCriticalSection);
			numWorkerThreads--;
			LeaveCriticalSection(&numWorkerThreadsCriticalSection);
			ExitThread(0);
			return 0;

		default:
			ExitThread(0); // TODO: critical error log and exit
		}
	}
}

///<summary> Closes the specified number of threads by queueing <c>POLYM_EVENT_SHUTDOWN</c> events. </summary>
///<returns> Returns the number of threads closed. </returns>
int closeWorkerThreads(int count)
{
	int countReturn = 0;
	for (int x = 0; x < count; x++)
	{
		countReturn += PostQueuedCompletionStatus(completionPort, 0, (ULONG_PTR)NULL, (OVERLAPPED*)&shutdownOverlap);
	}
	return countReturn;
}

///<summary> Gets the info object associated with the specified service ID. </summary>
/*POLYM_CONNECTION_INFO* getServiceConnectionInfo(int service_ID)
{
	EnterCriticalSection(&serviceConnectionsCriticalSection);
	POLYM_CONNECTION_INFO *ret = &connection_array_get(&serviceConnections, service_ID)->info;
	LeaveCriticalSection(&serviceConnectionsCriticalSection);
	return ret;
}*/

///<summary> Checks if the given service string is present in any of the connected services. </summary>
///<returns> Returns 1 on exact match, 0 otherwise. </returns>
int serviceStringExists(char* string) 
{
	POLYM_CONNECTION *connections[POLY_MAX_CONNECTIONS];
	EnterCriticalSection(&serviceConnectionsCriticalSection);
	int arraySize = connection_array_get_all(&serviceConnections, POLY_MAX_CONNECTIONS, connections);
	LeaveCriticalSection(&serviceConnectionsCriticalSection);
	for (int x = 0; x < arraySize; x++) 
	{
		if (strcmp(string, connections[x]->info.realm_info.service.serviceString))
			return 1;
	}
	return 0;
}

///<summary> Returns an array of pointers to the connection objects for all connected services. </summary>
///<param name="maxCount"> The maximum number of services to be retrieved. </param>
///<returns> Returns the number of services fetched </returns>
int getCurrentServiceConnections(void** OUT_connectionArray, unsigned int maxCount)
{
	EnterCriticalSection(&serviceConnectionsCriticalSection);
	int ret = connection_array_get_all(&serviceConnections, maxCount, (POLYM_CONNECTION**)OUT_connectionArray);
	LeaveCriticalSection(&serviceConnectionsCriticalSection);
	return ret;
}

///<summary> Returns an array of pointers to the connection objects for all connected peers. </summary>
///<param name="maxCount"> The maximum number of peers to be retrieved. </param>
///<returns> Returns the number of peers fetched. </returns>
int getCurrentPeerConnections(void** OUT_connectionArray, unsigned int maxCount)
{
	EnterCriticalSection(&peerConnectionsCriticalSection);
	int ret = connection_array_get_all(&peerConnections, maxCount, (POLYM_CONNECTION**)OUT_connectionArray);
	LeaveCriticalSection(&peerConnectionsCriticalSection);
	return ret;
}

///<summary> Returns an array of pointers to the connection objects for all connected clients. </summary>
///<param name="maxCount"> The maximum number of clients to be retrieved. </param>
///<returns> Returns the number of clients fetched. </returns>
int getCurrentClientConnections(void** OUT_connectionArray, unsigned int maxCount)
{
	EnterCriticalSection(&clientConnectionsCriticalSection);
	int ret = connection_array_get_all(&clientConnections, maxCount, (POLYM_CONNECTION**)OUT_connectionArray);
	LeaveCriticalSection(&clientConnectionsCriticalSection);
	return ret;
}

///<summary> Gets the address of a particular peer connection by ID, formatted as as string. </summary>
int getPeerConnectionAddressString(int peerID, int stringBufferSize, char* out_stringBuffer)
{
	EnterCriticalSection(&peerConnectionsCriticalSection);
	POLYM_CONNECTION* connection = connection_array_get(&peerConnections, peerID);
	LeaveCriticalSection(&peerConnectionsCriticalSection);
	switch (connection->addrtype)
	{
	case IPPROTO_IPV4:
		inet_ntop(AF_INET, &connection->address.ipv4.sin_addr, out_stringBuffer, stringBufferSize);
	case IPPROTO_IPV6:
		return -1; // ipv6 unimplemented
	default:
		return -1; // TODO: this should never happen, but throw an error just in case
	}
}

///<summary> Rebuilds the server's combined service string. </summary>
void rebuildServiceString()
{
	POLYM_CONNECTION *services[65536];
	int connectionCount = getCurrentServiceConnections((void**)&services, 65536);
	EnterCriticalSection(&serviceStringCriticalSection);
	strcpy(serviceString, "");
	for (int x = 0; x < connectionCount; x++)
	{
		strcat(serviceString, services[x]->info.realm_info.service.serviceString);
		strcat(serviceString, "|");
	}
	LeaveCriticalSection(&serviceConnectionsCriticalSection);
}

///<summary> Adds the supplied service to the service list. </summary>
///<param name="out_connectionPointer"> (OUT) The connection object added to the service list. </param>
///<returns> The index value of the element added. </return>
int addNewService(void* connection, void** out_connectionPointer)
{
	// TODO: need to fail if we have USHORT_MAX services without any vacancies. needs to return either negative error code or a value within the bounds of a USHORT.
	EnterCriticalSection(&serviceStringCriticalSection);
	strcat(serviceString, ((POLYM_CONNECTION*)connection)->info.realm_info.service.serviceString);
	strcat(serviceString, "|");
	LeaveCriticalSection(&serviceStringCriticalSection);
	EnterCriticalSection(&serviceConnectionsCriticalSection);
	int ret = connection_array_push(&serviceConnections, (POLYM_CONNECTION*)connection, (POLYM_CONNECTION**)out_connectionPointer);
	LeaveCriticalSection(&serviceConnectionsCriticalSection);
	return ret;
}

///<summary> Removes the service from the list as specified by the service ID. </summary>
///<returns> Integer result code. </returns>
int removeService(uint16_t serviceID)
{
	EnterCriticalSection(&serviceConnectionsCriticalSection);
	POLYM_CONNECTION* connection = connection_array_get(&serviceConnections, serviceID);

	if (connection = NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	connection_array_free(&serviceConnections, serviceID);
	LeaveCriticalSection(&serviceConnectionsCriticalSection);
	
	return closeConnection(connection);

	rebuildServiceString();
}

///<summary> Adds the supplied peer to the peer list. </summary>
///<param name="out_connectionPointer"> (OUT) The connection object added to the peer list. </param>
///<returns> The index value of the element added. </return>
int addNewPeer(void* connection, void **out_connectionPointer)
{
	EnterCriticalSection(&peerConnectionsCriticalSection);
	int ret = connection_array_push(&peerConnections, (POLYM_CONNECTION*)connection, (POLYM_CONNECTION**)out_connectionPointer);
	LeaveCriticalSection(&peerConnectionsCriticalSection);
	return ret;
}

///<summary> Removes the peer from the list as specified by the peer ID. </summary>
///<returns> Integer result code. </returns>
int removePeer(uint16_t peerID)
{
	EnterCriticalSection(&peerConnectionsCriticalSection);
	POLYM_CONNECTION* connection = connection_array_get(&peerConnections, peerID);

	if (connection = NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	connection_array_free(&peerConnections, peerID);
	LeaveCriticalSection(&peerConnectionsCriticalSection);

	return closeConnection(connection);
}

///<summary> Adds the supplied cleint to the client list. </summary>
///<param name="out_connectionPointer"> (OUT) The connection object added to the client list. </param>
///<returns> The index value of the element added. </return>
int addNewClient(void* connection, void **out_connectionPointer)
{
	EnterCriticalSection(&clientConnectionsCriticalSection);
	int ret = connection_array_push(&clientConnections, (POLYM_CONNECTION*)connection, (POLYM_CONNECTION**)out_connectionPointer);
	LeaveCriticalSection(&clientConnectionsCriticalSection);
	return ret;
}

///<summary> Removes the client from the list as specified by the client ID. </summary>
///<returns> Integer result code. </returns>
int removeClient(uint16_t clientID)
{
	EnterCriticalSection(&clientConnectionsCriticalSection);
	POLYM_CONNECTION* connection = connection_array_get(&clientConnections, clientID);

	if (connection = NULL)
		return POLY_ERROR_SERVICE_DOES_NOT_EXIST;

	connection_array_free(&clientConnections, clientID);
	LeaveCriticalSection(&clientConnectionsCriticalSection);

	return closeConnection(connection);
}

///<summary> Gets the connection object for the specified peer ID. </summary>
void* getConnectionFromPeerID(uint16_t peerID)
{
	EnterCriticalSection(&peerConnectionsCriticalSection);
	void *ret = connection_array_get(&peerConnections, peerID);
	LeaveCriticalSection(&peerConnectionsCriticalSection);
	return ret;
}

///<summary> Gets the connection object for the specified service ID. </summary>
void* getConnectionFromServiceID(uint16_t serviceID)
{
	EnterCriticalSection(&serviceConnectionsCriticalSection);
	void *ret = connection_array_get(&serviceConnections, serviceID);
	LeaveCriticalSection(&serviceConnectionsCriticalSection);
	return ret;
}

///<summary> Gets the connection object for the specified client ID. </summary>
void* getConnectionFromClientID(uint16_t clientID)
{
	EnterCriticalSection(&clientConnectionsCriticalSection);
	void *ret = connection_array_get(&clientConnections, clientID);
	LeaveCriticalSection(&clientConnectionsCriticalSection);
	return ret;
}

///<summary> Gets the connection info object from the given socket reference. </summary>
POLYM_CONNECTION_INFO* getInfoFromConnection(void *connection)
{
	return &((POLYM_CONNECTION*)connection)->info;
}

///<summary> Calculates the difference between two address structures. 0 means equality. </summary>
int compareAddresses(struct in_addr address1, struct in_addr address2)
{
	return address1.S_un.S_addr - address2.S_un.S_addr;
}

///<summary> Checks if a given address is already connected to the server. </summary>
int addressConnected(char *stringAddress, uint16_t port, uint8_t protocol)
{
	struct in_addr address1;
	inet_pton(AF_INET, stringAddress, &address1);
	POLYM_CONNECTION* connections[MAX_CONNECTIONS];
	int numConnections = getCurrentPeerConnections((void**)&connections, MAX_CONNECTIONS);

	for (int x = 0; x < numConnections; x++)
	{
		if (0 != compareAddresses(address1, connections[x]->address.ipv4.sin_addr) && port == connections[x]->address.ipv4.sin_port && protocol == connections[x]->protocol)
			return x;
	}
	return -1;
}

///<summary> Begin the process of initial.izing the supplied TCP connection. </summary>
void initializeNewTCPConnection(POLYM_CONNECTION *connection)
{
	const DWORD sock_timeout = 5 * 1000;
	setsockopt(connection->socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));
	setsockopt(connection->socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));
	connection->overlap.eventInfo = NULL;
	connection->overlap.eventType = 0;
	initializeOverlap(&connection->overlap);
	connection->buffer.len = 2;
	connection->buffer.buf = connection->bufferMemory;
	connection->overlap.hEvent = NULL;
	connection->byteCount = 0;
	connection->flags = MSG_WAITALL;
	connection->info.realm = POLY_REALM_UNINIT;
	connection->protocol = POLY_PROTO_TCP;
	connection->addrtype = IPPROTO_IPV4;
	InitializeCriticalSection(connection->connectionMutex);
}

///<summary> Open a new outbound connection using the TCP protocol. </summary>
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

	POLYM_CONNECTION connection;
	// Connect to server.
	connection.socket = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (connection.socket == SOCKET_ERROR) {
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

	initializeNewTCPConnection(&connection);
	connection.info.realm = POLY_REALM_PEER;

	*out_connectionInfo = &connection.info;

	POLYM_CONNECTION *connectionPointer;
	connection.info.realm_info.peer.peerID = addNewPeer(&connection, &connectionPointer);
	return connectionPointer;
}

///<summary> Open a new outbound connection on the given protocol. </summary>
void* openNewConnection(char *ipAddress, char *l4Port, POLYM_CONNECTION_INFO **out_connectionInfo, uint8_t protocol)
{
	switch (protocol)
	{
	case POLY_PROTO_TCP:
		return openNewTCPConnection(ipAddress, l4Port, out_connectionInfo);
	}
	return NULL;
}

///<summary> Loop that accepts new inbound connections and initializes them. </summary>
DWORD WINAPI acceptNewTCPConnections(LPVOID dummy)
{
	while (1)
	{

		// create a new empty connection object and wait for new connections
		int structlength = sizeof(struct sockaddr);
		POLYM_CONNECTION connection;
		connection.socket = accept(acceptSocket, (struct sockaddr*)&connection.address.ipv4, &structlength);
		
		// don't proceed if shutting down. accept will unblock but no connection will be present. break instead.
		if (isShutdown)
			break;

		// refuse connections if we're already at max.
		if (numConnections == maxConnections)
		{
			// send error code for full
			char buffer[2];
			insertShortIntoBuffer(buffer, POLY_ERROR_MAX_CONNECTIONS);
			send(connection.socket, buffer, 2, 0);
			shutdown(connection.socket, SD_BOTH);
			closesocket(connection.socket);
		}
		else
		{
			//set up the socket and put it on the IOCP listening queue
			printf("CONNECTIONS: %i\n", ++numConnections);
			initializeNewTCPConnection(&connection);
			

			// OPTI: initializing connections may need to become multithreaded.
			POLYM_CONNECTION *newConnectionPointer;
			if (POLY_REALM_FAILED == initializeIncomingConnection(&connection, &connection.info, &newConnectionPointer))
			{
				printf("CONNECTION FAILED.\n");
				closeConnection(&connection);
			}
			else
			{
				CreateTimerQueueTimer(&newConnectionPointer->checkAliveTimer, checkAliveTimerQueue, newConnectionTimeout, newConnectionPointer, newConnectionTimeoutInterval, newConnectionTimeoutInterval, 0);
				CreateIoCompletionPort((HANDLE)newConnectionPointer->socket, completionPort, (ULONG_PTR)newConnectionPointer, 0);
				WSARecv(newConnectionPointer->socket, &newConnectionPointer->buffer, 1, &newConnectionPointer->byteCount, &newConnectionPointer->flags, (OVERLAPPED*)&newConnectionPointer->overlap, NULL);
			}
		}
	}
	ExitThread(0);
	return 0;
}

///<summary> Begin a POLY server on the supplied port. </summary>
int startListenSocket(char* port) 
{
	initSocketLib();
	checkAliveTimerQueue = CreateTimerQueue();

	InitializeCriticalSection(&peerConnectionsCriticalSection);
	InitializeCriticalSection(&serviceConnectionsCriticalSection);
	InitializeCriticalSection(&serviceConnectionsCriticalSection);
	InitializeCriticalSection(&serviceStringCriticalSection);
	InitializeCriticalSection(&messageSpaceCriticalSection);
	InitializeCriticalSection(&numWorkerThreadsCriticalSection);

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	connection_array_init(&peerConnections);
	connection_array_init(&serviceConnections);
	connection_array_init(&clientConnections);
	message_buffer_array_init(&messageSpace, 100);

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

	EnterCriticalSection(&numWorkerThreadsCriticalSection);
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
	LeaveCriticalSection(&numWorkerThreadsCriticalSection);

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

//Tags: MUST_BE_FREED TEST_CASE_BUFFER
POLYM_MESSAGE_BUFFER* allocateBufferObject()
{
	message_buffer *message_buffer = message_buffer_array_allocate(&messageSpace);
	message_buffer->buffer_object.containerObject = message_buffer;
	return &message_buffer->buffer_object;
}