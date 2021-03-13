#include <cstdlib>
#include "pthread.h"
#include "proxy.h"
#include <string.h>
#include <fstream>

std::ofstream logFile_main("./proxy_server.log");
pthread_mutex_t mutex_main = PTHREAD_MUTEX_INITIALIZER;
int main() {
  const char * port = "12345";
  proxy * myproxy = new proxy(port);
  int listen_socket_fd = myproxy->setup_server();
  if (listen_socket_fd == -1) {
    pthread_mutex_lock(&mutex_main);
    logFile_main << "error in creating socket to connection accept" << std::endl;
    pthread_mutex_unlock(&mutex_main);
    return 1;
  }
  int id = 0;//to identify the client(start from 0, increment by one each thread&socket)
  while (1) {//continuously listen
    std::string ip;
    int comm_socket_fd = myproxy->server_accept(listen_socket_fd, &ip);
    if (comm_socket_fd == -1) {
      pthread_mutex_lock(&mutex_main);
      logFile_main << "error in connecting to client, current id = "<< id << std::endl;
      pthread_mutex_unlock(&mutex_main);
      continue;//new accept
    }
    pthread_t thread;
    pthread_mutex_lock(&mutex_main);
    client_info * curr_client = new client_info(comm_socket_fd, ip, id);
    id++;//increment thread id
    pthread_mutex_unlock(&mutex_main);
    pthread_create(&thread, NULL, proxy::process_request, curr_client);//new thread, using function process_request to handle client request
  }
  
}
