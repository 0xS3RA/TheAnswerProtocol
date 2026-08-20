#include "server.hpp"
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>


// 1. Créer un .receive_line dans le wrapper Socket pour le protocole réseau.
//    Il faut

void client_loop(Socket client_socket) {
   std::cout << "Connected to someone !" << std::endl;

   constexpr std::string_view welcome = "Welcome!";
   client_socket.send(welcome);

   constexpr std::string_view help_menu =
       "\nHelp menu :\n"
       "> :attack  - Attack an ennemy\n"
       "> :help    - Display this menu\n"
       "> :quit    - Quit the game and close the client\n";


   while (true) {
       auto message = client_socket.receive_line();
       if (message == "") return;

       std::cout << message;
       if (message == ":help\n") {
           std::cout << " > Received help menu request from " << client_socket.get() << std::endl;
           client_socket.send(help_menu);
       } else if (message == ":quit") {
           break;
       } else {
           std::cout << " > Received something from " << client_socket.get() << std::endl;
           client_socket.send("\nAccusé de réception");
       }
   }
   return;
}
