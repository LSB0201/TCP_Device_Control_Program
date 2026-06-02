#ifndef SERVER_H
#define SERVER_H

#include "common.h"

int init_tcp_server(int port);

void run_server_loop(int server_fd, DeviceState* state, volatile sig_atomic_t* keep_running);

void handle_client_request(int client_fd, DeviceState* state);

#endif //SERVER_H