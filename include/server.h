#ifndef SERVER_H
#define SERVER_H

#include "common.h"

int init_tcp_server(int port);

int handle_client_request(int client_fd, DeviceState* state);

void run_server_loop(int server_fd, DeviceState* state, volatile sig_atomic_t* keep_running);

#endif //SERVER_H