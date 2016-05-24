
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

// Complicated struct. Houses connection state and varios data structures required for IOCP to function.
typedef struct {
	SOCKET socket;
	int mode;
	int infoID; // the serviceID or peerID this socket represents
	WSABUF buffer;
	int byteCount;
	int flags;
	int request;
	WSAOVERLAPPED overlap;
	struct sockaddr address;
	int addrlen;
} CONNECTION_INFO ;

THREAD_INFO listenThread;
THREAD_INFO acceptThread;
CONNECTION_INFO* workQueue;
HANDLE completionPort;
int queuedJob = 0;
const int numWorkerThreads = 30;
const maxConnections = 500;
int numConnections = 0;
int isShutdown = 0;

CRITICAL_SECTION queueCritSection;
CONDITION_VARIABLE queueWorkAvailable;
CONDITION_VARIABLE queueWorkerAvailable;

int initSocketLib() 
{
	// Initialize Winsock
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int closeSocketLib()
{
	return WSACleanup();
}

// Wake thread and hand it a job. Will block until a thread is avilable if pool is saturated.
/*int enqueueJob(CONNECTION_INFO *job)
{
	EnterCriticalSection(&queueCritSection);
	while (workersAvailable == 0)
	{
		SleepConditionVariableCS(&queueWorkerAvailable, &queueCritSection, INFINITE);
	}
	workQueue = job;
	queuedJob = 1;
	LeaveCriticalSection(&queueCritSection);
	WakeConditionVariable(&queueWorkAvailable);
	return 0;
}*/

void closeConnection(CONNECTION_INFO *info)
{
	shutdown(info->socket, SD_BOTH);
	closesocket(info->socket);
	free(info->buffer.buf);
	free(info);
}

void handleTerminal(CONNECTION_INFO *job)
{

	int iSendResult;

	//iResult = recv(job->socket, recvbuf, 500, 0);
	if (job->byteCount > 0) {
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
		closeConnection(job);
	}
	else {
		printf("recv failed with error: %d\n", WSAGetLastError());
		WSACleanup();
	}

}

// Function that all worker threads will iterate
DWORD WINAPI workConnections(LPVOID dummy)
{

	while (!isShutdown) {
		int bytesTansferred;
		CONNECTION_INFO* connection;
		GetQueuedCompletionStatus(completionPort, &bytesTansferred, (PULONG_PTR)&connection, NULL, INFINITE);
		connection->byteCount = bytesTansferred;

		handleTerminal(connection);

		// determine source
		// process relevant data
		// sent requests ect
		// wait for work
	}

	ExitThread(0);
}

// Function for control thread that accepts new connections
DWORD WINAPI acceptNewConnections(LPVOID dummy)
{
	while (!isShutdown)
	{
		CONNECTION_INFO *info = malloc(sizeof(CONNECTION_INFO));
		info->socket = accept(acceptSocket, NULL, NULL);
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
			CreateIoCompletionPort((HANDLE)info->socket, completionPort, (ULONG_PTR)info, 0);
			WSARecv(info->socket, &info->buffer, 1, &info->byteCount, &info->flags, &info->overlap, NULL);
			numConnections++;
		}
	}
	ExitThread(0);
}

int startListenSocket(char* port) 
{
	initSocketLib();
	InitializeCriticalSectionAndSpinCount(&queueCritSection, 1000);
	InitializeConditionVariable(&queueWorkAvailable);
	InitializeConditionVariable(&queueWorkerAvailable);

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