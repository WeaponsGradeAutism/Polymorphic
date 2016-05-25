
// Network handling for WANGBLOWS

#define HAVE_STRUCT_TIMESPEC

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>

#define MODE_SERVICE 0
#define MODE_PEER 1

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
	int mode; // is this socket connected to a local service or a peer? MODE_SERVICE or MODE_PEER
	int infoID; // the serviceID or peerID this socket represents
	WSABUF buffer; // buffer where IOCP data is stored before completion packet is passed
	int request; // if a request was made by this peer, indicate it here so that worker threads can hadle properly when they receive a response
	HANDLE checkAliveTimer; // the timer that periodically checks to see if the connection is still alive
	int byteCount; // IOCP will put bytes buffered by response here. Although the wait function also returns the same thing. Basically used as a dummy for the IOCP create fucntion.
	int flags; // used by IOCP
	WSAOVERLAPPED overlap; // used by IOCP
	struct sockaddr address; // raw address daa provided by WCAAccept
	int addrlen; // raw address daa provided by WCAAccept
} CONNECTION_INFO ;

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

void closeConnection(CONNECTION_INFO *info)
{
	shutdown(info->socket, SD_BOTH);
	closesocket(info->socket);
	DeleteTimerQueueTimer(checkAliveTimerQueue, info->checkAliveTimer, INVALID_HANDLE_VALUE);
	free(info->buffer.buf);
	free(info);
}

int handleTerminal(CONNECTION_INFO *job)
{

	int iSendResult;

	//iResult = recv(job->socket, recvbuf, 500, 0);
	if (job->byteCount > 2) {
		printf("Bytes received: %d\n", job->byteCount);

		// Echo the buffer back to the sender
		iSendResult = send(job->socket, job->buffer.buf, job->byteCount, 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(job->socket);
			WSACleanup();
		}
		printf("Bytes sent: %d\n", iSendResult);
	}
	else if (job->byteCount < 3)
	{
		printf("Connection closing...\n");
		printf("CONNECTIONS: %i\n", --numConnections);
		closeConnection(job);
		return 0;
	}
	else {
		printf("recv failed with error: %d\n", WSAGetLastError());
		WSACleanup();
	}

	return 1;

} 

// Function that all worker threads will iterate
DWORD WINAPI workConnections(LPVOID dummy)
{

	while (1) {
		int bytesTansferred;
		CONNECTION_INFO *connection;
		OVERLAPPED *lap;

		GetQueuedCompletionStatus(completionPort, &bytesTansferred, (PULONG_PTR)&connection, &lap, INFINITE);

		if (isShutdown)
			break;

		ChangeTimerQueueTimer(checkAliveTimerQueue, connection->checkAliveTimer, checkAliveInterval, checkAliveInterval);

		connection->byteCount = bytesTansferred;

		if (handleTerminal(connection))
		{
			WSARecv(connection->socket, &connection->buffer, 1, &connection->byteCount, &connection->flags, &connection->overlap, NULL);
		}

		// determine source
		// process relevant data
		// sent requests ect
		// wait for work

	}

	ExitThread(0);
}


// WARN: We may need to synchronize this with sends on worker threads. Maybe not because send is blocking. I'm not sure.
// I imagine collisions will be very rare. If not synchronizing this causes a bug, it will be as rare.
VOID CALLBACK checkAlive(PVOID connection, BOOLEAN fired)
{
	printf("CHECK-ALIVE: %i\n", send(((CONNECTION_INFO*)connection)->socket, "still there?\n", 13, 0));
}

// Function for control thread that accepts new connections
DWORD WINAPI acceptNewConnections(LPVOID dummy)
{
	while (1)
	{
		CONNECTION_INFO *info = malloc(sizeof(CONNECTION_INFO));
		info->socket = accept(acceptSocket, NULL, NULL);
		if (isShutdown)
			break;
		if (numConnections == maxConnections)
		{
			send(info->socket, "fuck off we're full\n", 21, 0);
			closeConnection(info);
		}
		else
		{
			info->buffer.len = 500;
			info->buffer.buf = malloc(sizeof(char*) * 500);
			info->overlap.hEvent = NULL;
			info->byteCount = 0;
			info->flags = 0;
			CreateTimerQueueTimer(&info->checkAliveTimer, checkAliveTimerQueue, checkAlive, info, checkAliveInterval, checkAliveInterval, 0);
			CreateIoCompletionPort((HANDLE)info->socket, completionPort, (ULONG_PTR)info, 0);
			WSARecv(info->socket, &info->buffer, 1, &info->byteCount, &info->flags, &info->overlap, NULL);
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
		acceptNewConnections,
		NULL,
		0,
		&acceptThread.threadID
	);

	return 1;
}