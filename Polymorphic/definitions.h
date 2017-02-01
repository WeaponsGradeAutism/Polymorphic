#pragma once

//poly protocols
#define POLYM_PROTO_TCP 0
#define POLYM_PROTO_UDP 1
#define POLYM_PROTO_HTTP 2
#define POLYM_PROTO_HTTPS 3
#define POLYM_PROTO_SMTP 4

//poly modes
#define POLYM_MODE_UNINIT -1
#define POLYM_MODE_FAILED 0
#define POLYM_MODE_SERVICE 1
#define POLYM_MODE_PEER 2

//error codes
#define POLYM_ERROR_SERVICE_MAX_PEER_CONNECTIONS -2
#define POLYM_ERROR_PEER_MAX_SERVICE_CONNECTIONS -1
#define POLYM_ERROR_NOT_GRACEFUL 1
#define POLYM_ERROR_SERVICE_DOES_NOT_EXIST 2
#define POLYM_ERROR_MAX_CONNECTIONS 3
#define POLYM_ERROR_REALM_CODE_FAIL 4
#define POLYM_ERROR_SERVICE_STRING_SIZE_FAIL 6
#define POLYM_ERROR_INVALID_SERVICE_STRING_SIZE 7
#define POLYM_ERROR_SERVICE_STRING_FAIL 8
#define POLYM_ERROR_SERVICE_CREATION_FAIL 9
#define POLYM_ERROR_SERVICE_KEY_FAIL 10
#define POLYM_ERROR_SERVICE_KEY_INVALID 11
#define POLYM_ERROR_TRANSMISSION_FAIL 12
#define POLYM_ERROR_PEER_DOES_NOT_EXIST 13
#define POLYM_ERROR_CONNECTION_FAIL 13
#define POLYM_ERROR_INVALID_COMMAND 14
#define POLYM_ERROR_INVALID_ADDRESS 15
#define POLYM_ERROR_SERVICE_LIST_FULL 70000

#define POLY_COMMAND_CONNECT 0x0000
#define POLY_COMMAND_CONNECT_ERROR 0x0001
#define POLY_COMMAND_DISCONNECT 0x0001
#define POLY_COMMAND_PEER_DISCONNECTED 0x0001
#define POLY_COMMAND_MESSAGE 0x0002
#define POLY_COMMAND_MESSAGE_ERROR 0x0003

#define POLY_COMMAND_CONNECT_ERROR_BUSY 0x0001 // connection already exists, but the connection attempt timed out

#define POLYM_EVENT_LISTEN 1
#define POLYM_EVENT_ASYNC_SEND 2
#define POLYM_EVENT_SHUTDOWN 3