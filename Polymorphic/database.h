
#include <sqlite3.h>
#include <vector.h>
#include <stdbool.h>

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

#define VECTOR_INITIAL_CAPACITY 50

// service_info vector

typedef struct {
	int size;      // slots used so far
	int capacity;  // total available slots
	service_info *data;     // array of service_infos we're storing
} service_info_vector;

void si_vector_init(service_info_vector *vector);
void si_vector_init_capacity(service_info_vector *vector, int capacity);
void si_vector_append(service_info_vector *vector, service_info value);
service_info si_vector_get(service_info_vector *vector, int index);
bool si_vector_set(service_info_vector *vector, int index, service_info value);
void si_vector_trim(service_info_vector *vector);
void si_vector_free(service_info_vector *vector);

bool openDatabase(sqlite3** db);
int closeDatabase(sqlite3* db);
sqlite3* getCurrentDatabase();

int getPeerFromSocket(char* address, int port);
service_info_vector getServicesOnPeer(int peerID);
int_vector getPeersOnAddress(char* address);
peer_info getPeerInfoFromID(int peerID);
int_vector getPeersOnService(char* service);