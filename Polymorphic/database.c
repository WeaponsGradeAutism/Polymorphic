#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <database.h>
#include <stdlib.h>

#define newDBQuery "BEGIN TRANSACTION; \
CREATE TABLE polymorphic_info (key TEXT PRIMARY KEY NOT NULL, value TEXT) WITHOUT ROWID; \
CREATE TABLE peers (peerID INTEGER PRIMARY KEY, address INTEGER NOT NULL, port INTEGER NOT NULL, UNIQUE(address, port)); \
CREATE TABLE services_peers (peerID INTEGER NOT NULL, services TEXT NOT NULL); \
CREATE TABLE services_local (serviceID INTEGER PRIMARY KEY NOT NULL, service TEXT NOT NULL, access_token INTEGER NOT NULL) WITHOUT ROWID; \
CREATE TABLE users (userID INTEGER PRIMARY KEY, publicKeyVerify TEXT UNIQUE NOT NULL, publicKeyPM TEXT UNIQUE NOT NULL); \
CREATE TABLE user_peers (userID INTEGER NOT NULL, peerID INTEGER NOT NULL); \
CREATE TABLE proof_of_works (userID INTEGER NOT NULL, pow TEXT NOT NULL); \
\
INSERT INTO polymorphic_info (key, value) VALUES ('polymorphic_version', 0.1); \
\
CREATE INDEX services_peers_s_version ON services_peers (s_version); \
CREATE INDEX services_peers_provider ON services_peers (provider); \
CREATE INDEX services_peers_p_version ON services_peers (p_version); \
\
CREATE INDEX pows_users ON proof_of_works (userID); \
\
COMMIT;"

// table that houses peers requested by services

#define nonPersistentOptions "PRAGMA locking_mode = EXCLUSIVE;"

// --- BEGIN Data Structures --- //

// properly disposes of a service_info struct
void freeServiceInfo(service_info info)
{
	free(info.service);
	free(info.s_version);
	free(info.provider);
	free(info.p_version);
}

// properly disposes of a peer_info struct
void freePeerInfo(peer_info collection)
{
	free(collection.address);
}

// -- BEGIN service_info_vector functions -- //

void si_vector_init(service_info_vector *vector) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = VECTOR_INITIAL_CAPACITY;

	// allocate memory for vector->data
	vector->data = (service_info*)malloc(sizeof(service_info) * vector->capacity);
}

void si_vector_init_capacity(service_info_vector *vector, int capacity) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = capacity;

	// allocate memory for vector->data
	vector->data = (service_info*)malloc(sizeof(service_info) * vector->capacity);
}

void si_vector_double_capacity_if_full(service_info_vector *vector) {
	if (vector->size >= vector->capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->capacity *= 2;
		vector->data = (service_info*)realloc(vector->data, sizeof(service_info) * vector->capacity);
	}
}

void si_vector_append(service_info_vector *vector, service_info value) {
	// make sure there's room to expand into
	si_vector_double_capacity_if_full(vector);

	// append the value and increment vector->size
	vector->data[vector->size++] = value;
}

service_info si_vector_get(service_info_vector *vector, int index) {
	if (index >= vector->size || index < 0) {
		printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
		exit(1);
	}
	return vector->data[index];
}

bool si_vector_set(service_info_vector *vector, int index, service_info value) {
	// zero fill the vector up to the desired index
	while (index >= vector->size || index < 0) {
		return false;
	}

	// set the value at the desired index
	vector->data[index] = value;
	return true;
}

void si_vector_trim(service_info_vector *vector) {
	if (vector->size == 0)
	{
		free(vector->data);
		vector->data = NULL;
		vector->capacity = 0;
	}
	else
	{
		vector->data = (service_info*)realloc(vector->data, sizeof(service_info) * vector->size);
		vector->capacity = vector->size;
	}
}

void si_vector_free(service_info_vector *vector) {
	for (int row = 0; row < vector->size; row++) {
		freeServiceInfo(vector->data[row]);
	}
	free(vector->data);
}

// -- END service_info_vector functions -- //

// --- END Data Structures --- //

// --- BEGIN Database operations --- //

sqlite3_stmt* getPeerFromSocket_Stmt;
sqlite3_stmt* getPeersOnAddress_Stmt;
sqlite3_stmt* getPeerInfoFromID_Stmt;
sqlite3_stmt* getServicesOnPeer_Stmt;
sqlite3_stmt* getPeersOnService_Stmt;

sqlite3_stmt* getLocalServices;
sqlite3_stmt* getLocalServiceInfo;
sqlite3_stmt* getUserInfoFromID;
sqlite3_stmt* getUsersOnPeer;
sqlite3_stmt* getPeersWithUser;
sqlite3_stmt* getPOWsFromUser;

bool dbInit = false;
sqlite3* database;

//compiles statements
void compileStatements(sqlite3* db)
{
	sqlite3_prepare_v2(db, "SELECT peerID FROM peers WHERE address=?001 AND port=?002;", -1, &getPeerFromSocket_Stmt, NULL);
	sqlite3_prepare_v2(db, "SELECT peerID FROM peers WHERE address=?001;", -1, &getPeersOnAddress_Stmt, NULL);
	sqlite3_prepare_v2(db, "SELECT address,port FROM peers WHERE peerID=?001;", -1, &getPeerInfoFromID_Stmt, NULL);
	sqlite3_prepare_v2(db, "SELECT service,s_version,provider,p_version FROM services_peers WHERE peerID=?001;", -1, &getServicesOnPeer_Stmt, NULL);
	sqlite3_prepare_v2(db, "SELECT peerID FROM services_peers WHERE service=?001;", -1, &getPeersOnService_Stmt, NULL);
}

//desposes of compiled statements
void finalizeStatements(sqlite3* db)
{
	sqlite3_finalize(getPeerFromSocket_Stmt);
	sqlite3_finalize(getPeersOnAddress_Stmt);
	sqlite3_finalize(getPeerInfoFromID_Stmt);
	sqlite3_finalize(getServicesOnPeer_Stmt);
	sqlite3_finalize(getPeersOnService_Stmt);
}

//don't simply trust that any database opened is valid. do some consistency checks.
int checkValidDB(void* dbValid, int numResults, char** results, char** columns)
{
	*(bool*)dbValid = (numResults == 1) && (0 == strcmp(results[0], "polymorphic_version")); //check to see if there's indeed version info
	return 1; //cause the exec function to abort, as expected by openDatabase
}

//opens a valid polymorphic database if it can. opens existing database if it exists, if not, creates one. if an existing database is present but is not recognizable, returns false (fail).
bool openDatabase(sqlite3** db)
{

	//don't try to open two databases. that would be bad.
	if (database != NULL) 
	{
		printf("DATABASE ALREADY OPEN. ");
		return false;
	}

	//open the database connection
	if (SQLITE_OK != sqlite3_open("polymorphic.sql", db)) 
		return false; //failure

	//check to see if it's initialized
	bool dbValid;
	char* errmsg;
	int checkResult = sqlite3_exec(*db, "SELECT key FROM polymorphic_info WHERE key='polymorphic_version';", checkValidDB, &dbValid, &errmsg);

	//if it's a blank database, initialize it
	if (checkResult == SQLITE_ERROR)
	{
		if (0 == strcmp(errmsg, "no such table: polymorphic_info")) {
			printf("CREATING NEW DATABASE. ");
			checkResult = sqlite3_exec(*db, newDBQuery, NULL, NULL, &errmsg);
			if (checkResult != SQLITE_OK) {
				printf("UNRECOVERABLE ERROR IN FORMATTING DATABASE. ");
				printf("NUM: %i\n", checkResult);
				printf("STR: %s\n", errmsg);
				sqlite3_close(*db);
				return false;
			}
		}
		else
		{
			printf("UNRECOVERABLE ERROR: %s. ", errmsg);
			sqlite3_close(*db);
			return false;
		}
	}
	else if (checkResult == SQLITE_ABORT)
	{
		if (!dbValid)
		{
			printf("UNRECOGNIZED DATABASE FORMAT. ");
			sqlite3_close(*db);
			return false;
		}
	}
	else 
	{
		printf("UNRECOVERABLE SQL ERROR: %i. ", checkResult);
		sqlite3_close(*db);
		return false;
	}

	printf("PRECOMPILING STATEMENTS. ");
	compileStatements(*db);
	database = *db;
	return true;
}

// attempts to finalize all open objects on db handle and then close it.
int closeDatabase(sqlite3* db)
{
	finalizeStatements(db);
	database = NULL;
	return sqlite3_close(db);
}

// returns the current active database handle
sqlite3* getCurrentDatabase()
{
	return database;
}

// --- END Database operations --- //

// --- BEGIN I/O Functions --- ///

int_vector getPeersOnService(char* service)
{
	int_vector ret;
	int_vector_init(&ret);

	sqlite3_bind_text(getPeersOnService_Stmt, 1, service, strlen(service), SQLITE_STATIC);

	while (sqlite3_step(getPeersOnService_Stmt) == SQLITE_ROW)
	{
		int_vector_append(&ret, sqlite3_column_int(getPeersOnService_Stmt, 0));
	}
	int_vector_trim(&ret);
	sqlite3_reset(getPeersOnService_Stmt);
	return ret;
}

peer_info getPeerInfoFromID(int peerID)
{

	peer_info ret;
	sqlite3_bind_int(getPeerInfoFromID_Stmt, 1, peerID);

	if (sqlite3_step(getPeerInfoFromID_Stmt) == SQLITE_ROW)
	{
		int length;
		length = sqlite3_column_bytes(getPeerInfoFromID_Stmt, 0);
		ret.address = (char*)malloc(sizeof(char*) * (length + 1));
		strcpy(ret.address, (const char*)sqlite3_column_text(getPeerInfoFromID_Stmt, 0));
		ret.port = sqlite3_column_int(getPeerInfoFromID_Stmt, 1);
		length = sqlite3_column_bytes(getPeerInfoFromID_Stmt, 2);

		sqlite3_reset(getPeerInfoFromID_Stmt);
		return ret;
	}
	else
	{
		sqlite3_reset(getPeerInfoFromID_Stmt);
		ret.address = NULL;
		ret.port = 0;
		return ret;
	}
}

int_vector getPeersOnAddress(char* address)
{
	int_vector ret;
	int_vector_init(&ret);

	sqlite3_bind_text(getPeersOnAddress_Stmt, 1, address, strlen(address), SQLITE_STATIC);

	while (sqlite3_step(getPeersOnAddress_Stmt) == SQLITE_ROW)
	{
		int_vector_append(&ret, sqlite3_column_int(getPeersOnAddress_Stmt, 0));
	}
	int_vector_trim(&ret);
	sqlite3_reset(getPeersOnAddress_Stmt);
	return ret;
}

int getPeerFromSocket(char* address, int port)
{
	sqlite3_bind_text(getPeerFromSocket_Stmt, 1, address, strlen(address), SQLITE_STATIC);
	sqlite3_bind_int(getPeerFromSocket_Stmt, 2, port);

	if (sqlite3_step(getPeerFromSocket_Stmt) == SQLITE_ROW)
	{
		int ret = sqlite3_column_int(getPeerFromSocket_Stmt, 0);
		sqlite3_reset(getPeerFromSocket_Stmt);
		return ret;
	}
	else
	{
		sqlite3_reset(getPeerFromSocket_Stmt);
		return -1;
	}
}

service_info_vector getServicesOnPeer(int peerID)
{
	service_info_vector ret;
	si_vector_init(&ret);

	int length;

	sqlite3_bind_int(getServicesOnPeer_Stmt, 1, peerID);

	for (int row = 0; sqlite3_step(getServicesOnPeer_Stmt) == SQLITE_ROW; row++)
	{
		service_info draft;
		sqlite3_step(getServicesOnPeer_Stmt);
		length = sqlite3_column_bytes(getServicesOnPeer_Stmt, 0);
		draft.service = (char*)malloc(sizeof(char*) * (length + 1));
		strcpy(draft.service, (const char*)sqlite3_column_text(getServicesOnPeer_Stmt, 0));
		length = sqlite3_column_bytes(getServicesOnPeer_Stmt, 1);
		draft.s_version = (char*)malloc(sizeof(char*) * (length + 1));
		strcpy(draft.s_version, (const char*)sqlite3_column_text(getServicesOnPeer_Stmt, 1));
		length = sqlite3_column_bytes(getServicesOnPeer_Stmt, 2);
		draft.provider = (char*)malloc(sizeof(char*) * (length + 1));
		strcpy(draft.provider,  (const char*)sqlite3_column_text(getServicesOnPeer_Stmt, 2));
		length = sqlite3_column_bytes(getServicesOnPeer_Stmt, 3);
		draft.p_version = (char*)malloc(sizeof(char*) * (length + 1));
		strcpy(draft.p_version, (const char*)sqlite3_column_text(getServicesOnPeer_Stmt, 3));
	}

	sqlite3_reset(getServicesOnPeer_Stmt);
	return ret;
}

// --- END I/O Functions --- //