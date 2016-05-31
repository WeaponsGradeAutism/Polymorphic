

typedef struct {
	int mode; // is this socket connected to a local service or a peer? MODE_SERVICE or MODE_PEER
	int infoID; // the serviceID or peerID this socket represents
	char* address; // address string
	int port; // layer-4 port on the connection
	int encryptionType; // kind of encryption key used for this connection
	char* encryptionKey; // the encryption key used to decrypt traffic
} CONNECTION_INFO;

void processCommand(void* rawSocket, char* command, CONNECTION_INFO info);