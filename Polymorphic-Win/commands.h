#pragma once

#include <stdint.h>
#include <info_structure.h>

void processCommand(void *socket, uint8_t *command, POLYM_CONNECTION_INFO *connection_info);
