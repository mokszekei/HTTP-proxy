#include "response.hpp"
#include <string>
#include <stdio.h>

time_t Response::getTime(std::string input) {
    unsigned long start = input.find(", ")+1;
    unsigned long end = input.find(" GMT");
    input = input.substr(start, end - start);
    input = input.substr(input.find(", ") + 2);
    struct tm tm;
    std::time_t t;
    strptime(input.c_str(), "%d %b %Y %H:%M:%S", &tm);
    // std::cout << input.c_str() <<std::endl;
    t = mktime(&tm);
    return t;
}


void Response::parse(){
    std::string dateDelimiter = "Date: ";
    unsigned long dPos = response.find(dateDelimiter);
    if (dPos == std::string::npos) {
        date = time_t(nullptr);
    } else {
        date = getTime(response.substr(dPos+dateDelimiter.length(), response.find("\n") - dPos));
        // std::cout<< date <<std::endl;
    }
    

    std::string modifyDelimiter = "Last-Modified: ";
    unsigned long mPos = response.find(modifyDelimiter);
    if (mPos != std::string::npos) {
        last_modified = getTime(response.substr(mPos+modifyDelimiter.length(), response.find("\n") - mPos));
        // std::cout<< last_modified <<std::endl;
    }else{
        last_modified = time_t(nullptr);
    }
    

    size_t max_age_pos;
    if ((max_age_pos = response.find("max-age=")) != std::string::npos) {
        std::string max_age_str = response.substr(max_age_pos + 8);
        max_age = atoi(max_age_str.c_str());
    }else{
        max_age = -1;
    }

    std::string expiresDelimiter = "Expires: ";
    unsigned long ePos = response.find(expiresDelimiter);
    if (ePos != std::string::npos) {
        expire_time = getTime(response.substr(ePos+expiresDelimiter.length(), response.find("\n") - ePos));
    }else{
        expire_time = time_t(nullptr);
    }

    size_t nocatch_pos;
    if ((nocatch_pos = response.find("no-cache")) != std::string::npos) {
        nocache_flag = true;
    }else{
        nocache_flag = false;
    }

    size_t epos;
    if ((epos = response.find("ETag: ")) != std::string::npos) {
        size_t etag_end = response.find("\r\n", epos + 6);
        Etag = response.substr(epos + 6, etag_end - epos - 6);
    }else{
        Etag = "";
    }
}


void Response::parseLine() {
    size_t pos = response.find_first_of("\r\n");
    line = response.substr(0, pos);
}

bool Response::isChunk(){
  size_t pos;
  if ((pos = response.find("chunked")) != std::string::npos) {
    return true;
  }
  return false;
}

int Response::findContentL(){
  if(response.find("Content-Length")!=std::string::npos){
    int start=response.find("Content-Length");
    std::string sub=response.substr(start);
    int space=sub.find(' ');
    int end=sub.find('\r');
    std::string contsize=sub.substr(space+1,end-space);
    int ans=std::stoi(contsize);
    return ans;
  }
  if(response.find("Transfer-Encoding: chunked")!=std::string::npos){
    return -1;
  }
  return -2;
}

long Response::findLifetime(){
    if (response.find("Cache-Control: ") != std::string::npos) {
        if (response.find("no-cache") != std::string::npos){
            return 0;
        }
        return max_age;
    }

    if(expire_time != time_t(nullptr)){
        return expire_time - date;
    }
    
    if(last_modified != time_t(nullptr)){
        return (date-last_modified)/10;
    }
    
    return -1;
}
void Response::updateres(std::vector<char> res){
  std::string str ="";
  str.insert(str.begin(),res.begin(),res.end());
  response = str;
}
