#pragma once

//poly protocols
#define POLY_PROTO_TCP 0 // TCP protocol
#define POLY_PROTO_UDP 1 // UDP protocol
#define POLY_PROTO_HTTP 2 // HTTP protocol
#define POLY_PROTO_HTTPS 3 // HTTPS protocol
#define POLY_PROTO_SMTP 4 // SMTP protocol

//poly connection modes
#define POLY_MODE_UNINIT -1 // A connection in an established but uninitialized state
#define POLY_MODE_FAILED 0 // A connection that failed to initialize
#define POLY_MODE_SERVICE 1 // A service connection
#define POLY_MODE_PEER 2 // A peer connection

// internal error codes
#define POLYM_ERROR_NOT_GRACEFUL 1 // Connection failed to gracefully close

// connection initialization error codes
#define POLY_INIT_ERROR_REALM_CODE_FAIL 1 // Failed to recieve or process peer/service realm code
#define POLY_INIT_ERROR_MAX_CONNECTIONS 2 // This server cannot take on any more connections
#define POLY_INIT_ERROR_SERVICE_STRING_SIZE_FAIL 3 // Failed to recieve or process service string size
#define POLY_INIT_ERROR_INVALID_SERVICE_STRING_SIZE 4 // Supplied string was of invalid size
#define POLY_INIT_ERROR_SERVICE_STRING_FAIL 5 // Failed to recieve or process service string
#define POLY_INIT_ERROR_SERVICE_CREATION_FAIL 6 // Something failed in the addition of the service to the service list

// POLY protocol error codes
#define POLY_PROTO_ERROR_PEER_DOES_NOT_EXIST 1 // The requested peer ID does not exist on this server
#define POLY_PROTO_ERROR_TRANSMISSION_FAIL 2 // A socket operation failed
#define POLY_PROTO_ERROR_INVALID_COMMAND 4 // The supplied command code was invalid (TODO: disambiguate between globally invalid commands and commands that are invalid for this realm)
#define POLY_PROTO_ERROR_INVALID_ADDRESS 5 // The supplied IP address was not valid
#define POLY_PROTO_ERROR_SERVICE_DOES_NOT_EXIST 6 // The specified service does not exist on this server

// POLY command codes
#define POLY_COMMAND_CONNECT 0x0000
#define POLY_COMMAND_CONNECT_ERROR 0x0001
#define POLY_COMMAND_DISCONNECT 0x0001
#define POLY_COMMAND_PEER_DISCONNECTED 0x0001
#define POLY_COMMAND_MESSAGE 0x0002
#define POLY_COMMAND_MESSAGE_ERROR 0x0003

// POLY command error codes
#define POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL 0x0001 // The outbound connection failed

// Polymorphic thread events
#define POLYM_EVENT_LISTEN 1 // Socket being listened on has recieved an event
#define POLYM_EVENT_ASYNC_SEND 2 // An asynchronous send has completed
#define POLYM_EVENT_SHUTDOWN 3 // Signals a thread to shut down