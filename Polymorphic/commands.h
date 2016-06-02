#include <winsock2.h>

#define POLY_MODE_UNINIT -1
#define POLY_MODE_FAILED 0
#define POLY_MODE_SERVICE 1
#define POLY_MODE_SERVICE_NEW 2
#define POLY_MODE_PEER 3

#define POLY_PROTO_TCP 0
#define POLY_PROTO_UDP 1
#define POLY_PROTO_HTTP 2
#define POLY_PROTO_HTTPS 3
#define POLY_PROTO_SMTP 4

typedef struct {
	int protocol;
	int mode; // is this socket connected to a local service or a peer? POLY_MODE_SERVICE or POLY_MODE_PEER
	int infoID; // the serviceID or peerID this socket represents
	int addrtype; //IPv4 or V
	struct sockaddr address; // address string
	int port; // layer-4 port on the connection
	char serviceKey[4]; // key given to service on initialization
	char* serviceString; // the string that defines this service
} CONNECTION_INFO;

int sockRecv(void* socket, int protocol, char *buffer, int length);
int sockSend(void* socket, int protocol, char *buffer, int length);

void processCommand(void* rawSocket, char* command, CONNECTION_INFO *info);