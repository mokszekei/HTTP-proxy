#include "request.hpp"
#include <string>
#include <stdio.h>

void Request::findLine() {
    size_t pos = input.find_first_of("\r\n");
    line = input.substr(0, pos);
}

void Request::findMethod() {
    size_t mpos = input.find_first_of(" ");
    method = input.substr(0, mpos);
}

void Request::findHost() {
    if(input.find("Host: ")==std::string::npos){
        host="";
        port="";
    }
    size_t hpos = input.find("Host: ");
    std::string hostLine = input.substr(hpos+6);
    size_t end = hostLine.find_first_of("\r\n");
    hostLine = hostLine.substr(0, end);
    size_t end2 = hostLine.find(":");
    if(end2 != std::string::npos){
        host = hostLine.substr(0,end2);
        port = hostLine.substr(end2+1);

    }else{
        host = hostLine;
        port="80";
    }
}

// int Request::findContentL(){
//   if(input.find("Content-Length: ")!=std::string::npos){
//     int cpos = input.find("Content-Length: ");
//     std::string sub=input.substr(cpos);
//     int end=sub.find('\r');
//     std::string len=sub.substr(cpos+16,end-cpos-15);
//     int ans=std::stoi(len);
//     return ans;
//   }
//   return -1;
// }

int Request::findContentL(){
  if(input.find("Content-Length")!=std::string::npos){
    int start=input.find("Content-Length");
    std::string sub=input.substr(start);
    int space=sub.find(' ');
    int end=sub.find('\r');
    std::string contsize=sub.substr(space+1,end-space);
    int ans=std::stoi(contsize);
    return ans;
  }
  if(input.find("Transfer-Encoding: chunked")!=std::string::npos){
    return -1;
  }
  return -2;
}

std::string Request::getcurrTime(){
  time_t curr = time(0);
  char *tm = ctime(&curr);
  return tm;    
}

