#pragma once

#include <sqlite3.h>
#include <vector.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct  {
	char* service;
	char* s_version;
	char* provider;
	char* p_version;
} service_info;

typedef struct {
	char* address;
	int port;
	char* protocol;
} peer_info;

typedef struct {
	char* v_publickey;
	char* pm_publickey;
} user_info;

bool openDatabase(sqlite3** db);
int closeDatabase(sqlite3* db);
sqlite3* getCurrentDatabase();

int getPeerFromSocket(char* address, int port);
int getServicesOnPeer(int peerID, const char *stringOut, int32_t bufferSize);