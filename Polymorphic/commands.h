#pragma once

#include <stdint.h>
#include <info_structure.h>

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

#define POLY_COMMAND_CONNECT 0x0000
#define POLY_COMMAND_CONNECT_ERROR 0x0001
#define POLY_COMMAND_DISCONNECT 0x0001
#define POLY_COMMAND_PEER_DISCONNECTED 0x0001
#define POLY_COMMAND_MESSAGE 0x0002
#define POLY_COMMAND_MESSAGE_ERROR 0x0003

#define POLY_COMMAND_CONNECT_ERROR_BUSY 0x0001 // connection already exists, but the connection attempt timed out

#define POLY_SERVICE_ARRAY_GETALL_OVERFLOW -1

void processCommand(void *socket, uint8_t *command, POLYM_CONNECTION_INFO *connection_info);
