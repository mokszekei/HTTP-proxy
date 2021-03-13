#include <stdio.h>
#include <string>
#include <sstream>

class Request {
public:
    std::string input;
    std::string line;
    std::string host;
    std::string port;
    std::string method;
    
public:
    Request(){};
    Request(char * chars) {
        std::stringstream temp;
        temp.str(chars);
        input = temp.str();
        parse();
    };
    Request(std::string str){
        input = str;
        parse();
    };

    void parse(){
        findHost();
        findMethod();
        findLine();
    };
    void findHost();
    void findMethod();
    void findLine();
    
    int findContentL();
    std::string getcurrTime();
};
