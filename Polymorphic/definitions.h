#pragma once

//poly protocols
#define POLY_PROTO_TCP 0 // TCP protocol
#define POLY_PROTO_UDP 1 // UDP protocol
#define POLY_PROTO_HTTP 2 // HTTP protocol
#define POLY_PROTO_HTTPS 3 // HTTPS protocol
#define POLY_PROTO_SMTP 4 // SMTP protocol


//poly connection realms
#define POLY_REALM_UNINIT -2 // A connection in an established but uninitialized state
#define POLY_REALM_FAILED -1 // A connection that failed to initialize
#define POLY_REALM_PEER 0 // A peer connection
#define POLY_REALM_SERVICE 1 // A service connection
#define POLY_REALM_CLIENT 2 // A client connection

//poly encryption modes
#define POLY_ENC_NONE 0 // no encryption


//poly protocol facts
#define POLY_MAX_CONNECTIONS 65536 // The maximum number of connections allowable of either type
#define POLY_MAX_MESSAGE_SIZE 65546 // The maximum message size possible on the POLY protocol
#define POLY_MAX_SERVICE_STRING_SIZE 256 // The largest service string possible on the POLY protocol

#define POLY_COMMAND_HEADER_SIZE 4 // The offset past the command code at which the command begins
#define POLY_COMMAND_HEADER_CODE 0 // The header offset where the command code is stored
#define POLY_COMMAND_HEADER_ID 2 // The header offset where the identifier is stored


//BEGIN poly command info ------------------------

//CONNECT
#define POLY_COMMAND_CONNECT_MAX_SIZE_HEADLESS 8 // The maximum total size of a CONNECT_ERROR command without the message header
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS_HEADLESS 0 // The offset for the ipv4 address in CONNECT command without the message header
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT_HEADLESS 4 // The offset for the layer4 port in CONNECT command without the message header
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL_HEADLESS 6 // The offset for the protocol in CONNECT command without the message header
//non-headless
#define POLY_COMMAND_CONNECT_MAX_SIZE   (POLY_COMMAND_CONNECT_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a CONNECT_ERROR command
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS   (POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the ipv4 address in CONNECT command 
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT   (POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the layer4 port in CONNECT command 
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL   (POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the protocol in CONNECT command 

//CONNECT_ERROR
#define POLY_COMMAND_CONNECT_ERROR_MAX_SIZE_HEADLESS 10 // The maximum total size of a CONNECT_ERROR command without the message header
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS_HEADLESS 0 // The offset for the ipv4 address in CONNECT_ERROR command without the message header
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT_HEADLESS 4 // The offset for the layer4 port in CONNECT_ERROR command without the message header
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL_HEADLESS 6 // The offset for the protocol in CONNECT_ERROR command without the message header
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_ERROR_CODE_HEADLESS 8 // The offset for the error code in CONNECT_ERROR command without the message header
//non-headless
#define POLY_COMMAND_CONNECT_ERROR_MAX_SIZE   (POLY_COMMAND_CONNECT_ERROR_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a CONNECT_ERROR command
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS   (POLY_COMMAND_CONNECT_ERROR_OFFSET_IPV4_ADDRESS_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the ipv4 address in CONNECT_ERROR command 
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT   (POLY_COMMAND_CONNECT_ERROR_OFFSET_LAYER4_PORT_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the layer4 port in CONNECT_ERROR command 
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL   (POLY_COMMAND_CONNECT_ERROR_OFFSET_PROTOCOL_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the protocol in CONNECT_ERROR command 
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_ERROR_CODE   (POLY_COMMAND_CONNECT_ERROR_OFFSET_ERROR_CODE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the error code in CONNECT_ERROR command 

//DISCONNECT
#define POLY_COMMAND_DISCONNECT_MAX_SIZE_HEADLESS 2 // The maximum total size of a DISCONNECT command
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_DISCONNECT_CODE_HEADLESS 0 // The offset for the disconnect code in CONNECT_ERROR command 
//non-headless
#define POLY_COMMAND_DISCONNECT_MAX_SIZE   (POLY_COMMAND_DISCONNECT_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a DISCONNECT command
#define POLY_COMMAND_CONNECT_ERROR_OFFSET_DISCONNECT_CODE   (POLY_COMMAND_CONNECT_ERROR_OFFSET_DISCONNECT_CODE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the disconnect code in CONNECT_ERROR command 

//MESSAGE
#define POLY_COMMAND_MESSAGE_MAX_SIZE_HEADLESS 65544 // The maximum total size of a MESSAGE command without the message header
#define POLY_COMMAND_MESSAGE_OFFSET_PEER_ID_HEADLESS 0 // The offset for the peer ID in MESSAGE command without the message header
#define POLY_COMMAND_MESSAGE_OFFSET_SOURCE_ID_HEADLESS 2 // The offset for the source ID in MESSAGE command without the message header
#define POLY_COMMAND_MESSAGE_OFFSET_DESTINATION_ID_HEADLESS 4 // The offset for the destination ID in MESSAGE command without the message header
#define POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH_HEADLESS 6 // The offset for the message length in MESSAGE command without the message header
#define POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_HEADLESS 8 // The offset for the the message in MESSAGE command without the message header
//non-headless
#define POLY_COMMAND_MESSAGE_MAX_SIZE   (POLY_COMMAND_MESSAGE_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a MESSAGE command
#define POLY_COMMAND_MESSAGE_OFFSET_PEER_ID   (POLY_COMMAND_MESSAGE_OFFSET_PEER_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the peer ID in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_SOURCE_ID   (POLY_COMMAND_MESSAGE_OFFSET_SOURCE_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the source ID in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_DESTINATION_ID   (POLY_COMMAND_MESSAGE_OFFSET_DESTINATION_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the destination ID in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH   (POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_LENGTH_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the message length in MESSAGE command 
#define POLY_COMMAND_MESSAGE_OFFSET_MESSAGE   (POLY_COMMAND_MESSAGE_OFFSET_MESSAGE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the the message in MESSAGE command 

//MESSAGE_SERVICE
#define POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE_HEADLESS 65542 // The maximum total size of a MESSAGE_SERVICE command without the message header
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID_HEADLESS 0 // The offset for the source ID in MESSAGE_SERVICE command without the message header
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_DESTINATION_ID_HEADLESS 2 // The offset for the destination ID in MESSAGE_SERVICE command without the message header
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_LENGTH_HEADLESS 4 // The offset for the message length in MESSAGE_SERVICE command without the message header
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_HEADLESS 6 // The offset for the message in MESSAGE_SERVICE command without the message header
//non-headless
#define POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE   (POLY_COMMAND_MESSAGE_SERVICE_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a MESSAGE_SERVICE command
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID   (POLY_COMMAND_MESSAGE_SERVICE_OFFSET_SOURCE_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the source ID in MESSAGE_SERVICE command
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_DESTINATION_ID   (POLY_COMMAND_MESSAGE_SERVICE_OFFSET_DESTINATION_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the destination ID in MESSAGE_SERVICE command
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_LENGTH   (POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_LENGTH_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the message length in MESSAGE_SERVICE command
#define POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE   (POLY_COMMAND_MESSAGE_SERVICE_OFFSET_MESSAGE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the message in MESSAGE_SERVICE command

//MESSAGE_CLIENT
#define POLY_COMMAND_MESSAGE_CLIENT_MAX_SIZE_HEADLESS 65542 // The maximum total size of a MESSAGE_CLIENT command without the message header
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID_HEADLESS 0 // The offset for the source ID in MESSAGE_CLIENT command without the message header
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID_HEADLESS 2 // The offset for the destination ID in MESSAGE_CLIENT command without the message header
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH_HEADLESS 4 // The offset for the message length in MESSAGE_CLIENT command without the message header
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_HEADLESS 6 // The offset for the message in MESSAGE_CLIENT command without the message header
//non-headless
#define POLY_COMMAND_MESSAGE_CLIENT_MAX_SIZE   (POLY_COMMAND_MESSAGE_CLIENT_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a MESSAGE_CLIENT command 
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID   (POLY_COMMAND_MESSAGE_CLIENT_OFFSET_SOURCE_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the source ID in MESSAGE_CLIENT command 
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID   (POLY_COMMAND_MESSAGE_CLIENT_OFFSET_DESTINATION_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the destination ID in MESSAGE_CLIENT command 
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH   (POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_LENGTH_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the message length in MESSAGE_CLIENT command 
#define POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE   (POLY_COMMAND_MESSAGE_CLIENT_OFFSET_MESSAGE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the message in MESSAGE_CLIENT command 

//ERROR
#define POLY_COMMAND_ERROR_MAX_SIZE_HEADLESS 2 // The maximum total size of a ERROR command without the message header
#define POLY_COMMAND_ERROR_OFFSET_ERROR_CODE_HEADLESS 0 // The offset for the error code in ERROR command without the message header
//non-headless
#define POLY_COMMAND_ERROR_MAX_SIZE   (POLY_COMMAND_ERROR_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a ERROR command
#define POLY_COMMAND_ERROR_OFFSET_ERROR_CODE   (POLY_COMMAND_ERROR_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the error code in ERROR command 

//PEER_DISCONNECTED
#define POLY_COMMAND_PEER_DISCONNECTED_MAX_SIZE_HEADLESS 2 // The maximum total size of a PEER_DISCONNECTED command without the message header
#define POLY_COMMAND_PEER_DISCONNECTED_OFFSET_PEER_ID_HEADLESS 0 // The offset for the peer ID in PEER_DISCONNECTED command without the message header
//non-headless
#define POLY_COMMAND_PEER_DISCONNECTED_MAX_SIZE   (POLY_COMMAND_PEER_DISCONNECTED_MAX_SIZE_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The maximum total size of a PEER_DISCONNECTED command
#define POLY_COMMAND_PEER_DISCONNECTED_OFFSET_PEER_ID   (POLY_COMMAND_PEER_DISCONNECTED_OFFSET_PEER_ID_HEADLESS + POLY_COMMAND_HEADER_SIZE) // The offset for the peer ID in PEER_DISCONNECTED command 

//END poly command info --------------------------


// internal error codes
#define POLYM_ERROR_NOT_GRACEFUL 1 // Connection failed to gracefully close


// POLY protocol error codes
#define POLY_ERROR_PEER_DOES_NOT_EXIST 1 // The requested peer ID does not exist on this server
#define POLY_ERROR_SERVICE_DOES_NOT_EXIST 2 // The requested peer ID does not exist on this server
#define POLY_ERROR_CLIENT_DOES_NOT_EXIST 3 // The requested peer ID does not exist on this server
#define POLY_ERROR_TRANSMISSION_FAIL 4 // A socket operation failed
#define POLY_ERROR_INVALID_COMMAND 5 // The supplied command code was invalid
#define POLY_ERROR_DISALLOWED_COMMAND 6 // the supplied command code isn't allowed for this realm.
#define POLY_ERROR_INVALID_ADDRESS 7 // The supplied IP address was not valid
#define POLY_ERROR_MAX_CONNECTIONS 8 // The server cannot accept any more connections


// POLY command codes
#define POLY_COMMAND_CONNECT 0x0000
#define POLY_COMMAND_CONNECT_ERROR 0x0001
#define POLY_COMMAND_DISCONNECT 0x0002
#define POLY_COMMAND_PEER_DISCONNECTED 0x0003
#define POLY_COMMAND_MESSAGE 0x0004
#define POLY_COMMAND_MESSAGE_SERVICE 0x0005
#define POLY_COMMAND_MESSAGE_CLIENT 0x0006
#define POLY_COMMAND_ERROR 0x0007


// POLY command error codes
#define POLY_COMMAND_CONNECT_ERROR_CONNECTION_FAIL 0x0001 // The outbound connection failed


// Polymorphic thread events
#define POLYM_EVENT_ACCEPT 0 // Socket is being initialized
#define POLYM_EVENT_LISTEN 1 // Socket being listened on has recieved an event
#define POLYM_EVENT_ASYNC_SEND 2 // An asynchronous send has completed
#define POLYM_EVENT_SHUTDOWN 3 // Signals a thread to shut down

