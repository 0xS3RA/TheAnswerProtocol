#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#include "common/Socket.hpp"


void server_loop(Socket socket);
