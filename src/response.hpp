#include <stdio.h>
#include <string>
#include <sstream>
#include <ctime>
#include <iostream>
#include <vector>

class Response {
public:
  std::string response;
  std::string line;
  std::string Etag;
  time_t last_modified;
  int max_age;
  time_t expire_time;
  time_t date;
  bool nocache_flag;


public:
  Response(){};
  Response(std::string str){
        response = str;
        parse();
        parseLine();
    };
  
  time_t getTime(std::string input);
  void parse();
  void parseLine();
  
  int findContentL();
  bool isChunk();
  long findLifetime();
  void updateres(std::vector<char> res);
};


// int main(){
//   Response test("HTTP/1.1 200 OK\r\nDate: Sun, 10 Oct 2010 23:26:07 GMT\r\nServer: Apache/2.2.8 (Ubuntu) mod_ssl/2.2.8 OpenSSL/0.9.8g\r\nLast-Modified: Sun, 26 Sep 2010 22:04:35 GMT\r\nETag: '45b6-834-49130cc1182c0'\r\nAccept-Ranges: bytes\r\nContent-Length: 12\r\nConnection: close\r\n\r\nContent-Type: text/html");
//   std::cout<< test.Etag <<std::endl;
//   std::cout<< test.last_modified <<std::endl;
//   std::cout<< test.date <<std::endl;
//   std::cout<< test.line <<std::endl;
//   std::cout<< test.max_age <<std::endl;
//   std::cout<< test.expire_time <<std::endl;
//   std::cout<< test.nocache_flag <<std::endl;
//   std::cout<< test.findLifetime() <<std::endl;
//   std::cout<< test.isChunk() <<std::endl;
  
//   return EXIT_SUCCESS;
// }
