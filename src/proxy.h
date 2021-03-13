#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "request.hpp"
#include "response.hpp"
#include "client_info.h"

class proxy{
 private:
  const char * portNum;
 public:
 proxy(const char * portN) : portNum(portN){}
  /*get proxy server file descripter*/
  int setup_server();
  /*get real server file descripter based on parsed host and port number*/
  static int setup_client(const char * host, const char * port);
  /*get client file descripter based on server port number, also set ip*/
  static int get_client_fd();
  /*accept on listen socket and return a new socket fd to handle this client*/
  int server_accept(int socket_fd, std::string * ip);
  /*process the client request based on client_info(socket,ip,id)*/
  static void * process_request(void * client_info);
  static void * handleGET(Request *req, client_info client, int client_fd, ssize_t len, int server_fd);
  static void * handlePOST(int client_fd, int server_fd, int id, std::string host, std::string line, int req_size, std::string msg);
  static void * handleConnect(int client_fd, int server_fd, int id, std::string host, std::string line);
  static void send_cacheinfo(Response res, int client_fd, int ID);
  static void send_server(Request* req, int server_fd, int client_fd, int ID, std::string host);
  static void print_cacheinfo(Response *res, bool no_store, std::string line, int ID, int max_capacity);
  static bool revalidate(Response *res, std::string req, int server_fd,int ID);
  static bool checkExp(int server_fd, Response *res, Request *req, int ID);
  static void cacheprinter();
  static void handle502(int client_fd);
};
