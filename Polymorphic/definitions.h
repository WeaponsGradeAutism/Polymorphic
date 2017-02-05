#pragma once

//poly protocols
#define POLY_PROTO_TCP 0 // TCP protocol
#define POLY_PROTO_UDP 1 // UDP protocol
#define POLY_PROTO_HTTP 2 // HTTP protocol
#define POLY_PROTO_HTTPS 3 // HTTPS protocol
#define POLY_PROTO_SMTP 4 // SMTP protocol


//poly connection realms
#define POLY_REALM_UNINIT -1 // A connection in an established but uninitialized state
#define POLY_REALM_FAILED 0 // A connection that failed to initialize
#define POLY_REALM_PEER 0 // A peer connection
#define POLY_REALM_SERVICE 1 // A service connection
#define POLY_REALM_CLIENT 2 // A client connection


//poly protocol facts
#define POLY_MAX_CONNECTIONS 65536 // The maximum number of connections allowable of either type
#define POLY_MAX_MESSAGE_SIZE 65546 // The maximum message size possible on the POLY protocol
#define POLY_COMMAND_OFFSET 2 // The offset past the command code at which the command begins


//BEGIN poly command info ------------------------

//MESSAGE
#define POLY_COMMAND_MESSAGE_MAX_SIZE 65546 // The maximum total size of a MESSAGE command
#define POLY_COMMAND_MESSAGE_OFFSET_PEER_ID 2 // The offset for the peer ID in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_SOURCE_ID 4 // The offset for the source ID in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_DESTINATION_ID 6 // The offset for the destination ID in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH 8 // The offset for the message length in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_MESSAGE 10 // The offset for the the message in MESSAGE command 

//MESSAGE_SERVICE
#define POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE 65544 // The maximum total size of a MESSAGE_SERVICE command
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID 2 // The offset for the source ID in message MESSAGE_SERVICE command 
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_DESTINATION_ID 4 // The offset for the destination ID in message MESSAGE_SERVICE command 
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_LENGTH 6 // The offset for the message length in message MESSAGE_SERVICE command 
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE 8 // The offset for the message in message MESSAGE_SERVICE command 

//MESSAGE_CLIENT
#define POLY_COMMAND_MESSAGE_CLIENT_MAX_SIZE 65544 // The maximum total size of a MESSAGE_CLIENT command
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID 2 // The offset for the source ID in message MESSAGE_CLIENT command 
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID 4 // The offset for the destination ID in message MESSAGE_CLIENT command 
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH 6 // The offset for the message length in message MESSAGE_CLIENT command 
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE 8 // The offset for the message in message MESSAGE_CLIENT command 

//END poly command info --------------------------


// internal error codes
#define POLYM_ERROR_NOT_GRACEFUL 1 // Connection failed to gracefully close


// POLY protocol error codes
#define POLY_PROTO_ERROR_PEER_DOES_NOT_EXIST 1 // The requested peer ID does not exist on this server
#define POLY_PROTO_ERROR_SERVICE_DOES_NOT_EXIST 2 // The requested peer ID does not exist on this server
#define POLY_PROTO_ERROR_CLIENT_DOES_NOT_EXIST 3 // The requested peer ID does not exist on this server
#define POLY_PROTO_ERROR_TRANSMISSION_FAIL 4 // A socket operation failed
#define POLY_PROTO_ERROR_INVALID_COMMAND 5 // The supplied command code was invalid (TODO: disambiguate between globally invalid commands and commands that are invalid for this realm)
#define POLY_PROTO_ERROR_INVALID_ADDRESS 6 // The supplied IP address was not valid
#define POLY_PROTO_ERROR_MAX_CONNECTIONS 7 // The server cannot accept any more connections


// POLY command codes
#define POLY_COMMAND_CONNECT 0x0000
#define POLY_COMMAND_CONNECT_ERROR 0x0001
#define POLY_COMMAND_DISCONNECT 0x0002
#define POLY_COMMAND_PEER_DISCONNECTED 0x0003
#define POLY_COMMAND_MESSAGE 0x0004
#define POLY_COMMAND_MESSAGE_SERVICE 0x0005
#define POLY_COMMAND_MESSAGE_CLIENT 0x0006
#define POLY_COMMAND_MESSAGE_ERROR 0x0007


// POLY command error codes
#define POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL 0x0001 // The outbound connection failed


// Polymorphic thread events
#define POLYM_EVENT_LISTEN 1 // Socket being listened on has recieved an event
#define POLYM_EVENT_ASYNC_SEND 2 // An asynchronous send has completed
#define POLYM_EVENT_SHUTDOWN 3 // Signals a thread to shut down