#include "server.hpp"
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

void client_loop(Socket &client_socket) {
   std::cout << "Connected to someone !" << std::endl;

   write(client_socket.get(), "Welcome !", 9);

   std::string help_menu {"Help menu : \n:attack  - Attack an ennemy\n"};

   char client_response[1024];
   char server_response[1024];

   memset(client_response, 0, 1023);
   memset(server_response, 0, 1023);

   while (true) {
       recv(client_socket.get(), client_response, sizeof(client_response), 0);
       std::cout << client_response;
       if (strcmp(client_response, ":help") == 0) {
           std::cout << "> Received help menu request" << std::endl;
           write(client_socket.get(), help_menu.c_str(), help_menu.length());
       } else {
           std::cout << "> Received something" << std::endl;
           write(client_socket.get(), "Caca", 4);
       }
   }
   return;
}
