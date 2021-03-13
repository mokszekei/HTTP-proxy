#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;
class client_info {
 private:
  int id;
  int socket_fd;
  string ip;
 public:
 client_info(int socket_fd_, string ip_, int id_) : socket_fd(socket_fd_), ip(ip_),id(id_){}
  int getFd() { return socket_fd; }
  string getIP() { return ip; }
  int getID() { return id; }
};
