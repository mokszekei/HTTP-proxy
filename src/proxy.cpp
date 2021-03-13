#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <unordered_map>
#include "proxy.h"

std::unordered_map<std::string, Response> Cache;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
std::ofstream logFile("./proxy_server.log");


int proxy::setup_server(){
  const char * port = this->portNum;
  const char * hostname = NULL;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  int status;
  int socket_fd;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << " ," << port << ")" << endl;
    return -1;
  }

  if (strcmp(port, "") == 0) {
    struct sockaddr_in * addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
    addr_in->sin_port = 0;
  }

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  freeaddrinfo(host_info_list);
  return socket_fd;
}


int proxy::server_accept(int socket_fd, std::string * ip) {
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connect_fd;

  client_connect_fd =
      accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connect_fd == -1) {
    cerr << "Cannot accept connection on socket" << endl;
    return -1;
  }
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  *ip = inet_ntoa(addr->sin_addr);

  return client_connect_fd;
}

int proxy::setup_client(const char * hostname, const char * port) {
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  int status;
  int server_socket_fd;


  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  //get address info
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  //build socket that commu with real server
  server_socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (server_socket_fd == -1) {
    cerr << "Cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  status = connect(server_socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  std::cout << "Connect to server successfully\n";
  cout<< "  (" << hostname << "," << port << ")" <<endl;
  freeaddrinfo(host_info_list);
  return server_socket_fd;//return the server socket file descr
}

void * proxy::process_request(void * cl_info){
  client_info * cli = (client_info*) cl_info;
  int comm_socket_fd = cli->getFd();//socket to commu with client
  int ID = cli->getID();
  char req_msg[65536] = {0};
  ssize_t len = recv(comm_socket_fd, req_msg, sizeof(req_msg), 0);
  if (len <= 0) {
    pthread_mutex_lock(&mutex);
    logFile << cli->getID() << ": WARNING Invalid Request" << std::endl;
    pthread_mutex_unlock(&mutex);
    return NULL;
  }
  std::string input = std::string(req_msg, len);//convert request from char array to std::string
  //parsing request(input)
  Request* request_info = new Request(input);
  //test messages std::cout
  // logFile << cli->getID() << ": test message-FENG WANG" << std::endl;//to be removed
  //cout<<"request string: "<<input<<endl;
  //cout<<"request method: "<<request_info->method<<endl;
  //parse input
  //assume Get server's host and port and request method
  //build the socket to comm with real server
  
  int server_socket_fd = setup_client(request_info->host.c_str(), request_info->port.c_str());//to be obtained
  if(server_socket_fd == -1){
  	close(server_socket_fd);
  	close(comm_socket_fd);
  	pthread_mutex_lock(&mutex);
    	logFile<<"Not a valid request! Close sockets"<<std::endl;
    	pthread_mutex_unlock(&mutex);
  	return NULL;
  }
  if(request_info ->method != "GET" && request_info->method != "POST" && request_info->method != "CONNECT"){
    pthread_mutex_lock(&mutex);
    logFile<<cli->getID()<<": Responding \""<<"HTTP/1.1 400 Bad Request\""<<std::endl;
    pthread_mutex_unlock(&mutex);
  }
  
  if (request_info->method == "GET") {//subsitute parsed_request
    //send(server_socket_fd, req_msg, len, 0);
    handleGET(request_info, *cli, comm_socket_fd, len, server_socket_fd);
  }

  if (request_info->method == "CONNECT") {
    handleConnect(comm_socket_fd, server_socket_fd, ID, request_info->host.c_str(), request_info->line.c_str());
  }

  if (request_info->method == "POST") {
    handlePOST(comm_socket_fd, server_socket_fd, ID, request_info->host.c_str(), request_info->line.c_str(), len, input);
  }
  close(server_socket_fd);
  close(comm_socket_fd);
  
  return NULL;
}


void* proxy::handleConnect(int client_fd, int server_fd, int id,  std::string host, std::string line){
           //construct connection with server
    // int server_fd = goServer(request.getHost().c_str(), request.getPort().c_str());

    //write requesting form server
    logFile << id << ": "
            << "Requesting \"" << line << "\" from " << host << std::endl;

    //send a 200K to the client
    const char* ack_msg = "HTTP/1.1 200 OK\r\n\r\n";
    while(send(client_fd, ack_msg, strlen(ack_msg), 0) < 0){
        cerr << "error: failed to send 200 OK" << endl;
    }

    //transfer massages between client and server until some side close the connection
    fd_set readfds;
    int max_sd = max(client_fd, server_fd);
    while(1){
        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);
        FD_SET(server_fd, &readfds);
        //wait for an activity on one of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if(activity < 0 ){
            cerr << "error: select" << endl;
            break;
        }
        if(FD_ISSET(client_fd, &readfds)){//if get request from client
            vector<char> buffer(65535, 0);
            size_t rev_sz = recv(client_fd, &(buffer[0]), 65534, 0);
            if(rev_sz == 0) {
                break;
            }
            //send received request to the server
            size_t tmp_sz = send(server_fd, buffer.data(), rev_sz, 0);
            if(tmp_sz == 0) break;
        }
        else if(FD_ISSET(server_fd, &readfds)){//if get response from server
            vector<char> buffer(65535, 0);
            size_t rev_sz = recv(server_fd, &(buffer[0]), 65534, 0);
            if(rev_sz < 0){
              handle502(client_fd);
              logFile << id << ": ERROR 502 : proxy cannot get valid message from server" << std::endl;
            }
            if(rev_sz == 0) {
                break;
            }
            //send received response to the client
            size_t tmp_sz = send(client_fd, buffer.data(), rev_sz, 0);
            if(tmp_sz == 0) break;
        }
    }
    close(server_fd);
    // Writter::write(log1);
    logFile << id << ": Tunnel closed" << std::endl;
    return NULL;
}

void* proxy::handlePOST(int client_fd, int server_fd, int id, std::string host, std::string line, int req_size, std::string msg){

  logFile << id << ": "
        << "Requesting \"" << line << "\" from " << host << std::endl;

  cout<<"In handlePOST function"<<endl;
  cout<<"msg: "<< msg <<endl;
  cout<<"req_size "<< req_size <<endl;
  // request also need to check content length
  Request req(msg);
  int content_len = req.findContentL();
  cout<<"check content length: "<< req.findContentL() <<endl;

  // make sure that we reveive all request from client
  // int total_len = 0;
  // int len = 0;
  // while (total_len < content_len) {
  //     char new_server_msg[65536] = {0};
  //     if ((len = recv(client_fd, new_server_msg, sizeof(new_server_msg), 0)) <= 0) {
  //       cout<<"no more msg"<<endl;
  //       break;  
  //     }
  //     std::string temp(new_server_msg, len);
  //     cout<<"new_server_msg： "<<temp<<endl;
  //     msg += temp;
  //     total_len += len;
  // }

  // cout<<"complete request string"<<msg<<endl;

  //send the request to server
  send(server_fd, msg.c_str(), msg.size(),0);

  cout<<"Already sent request to server."<<endl;

  // receive response from server
  std::vector<char> res_buffer(50000,0);
  int index=0;
  int res_sz=0;
  
  res_sz = recv(server_fd, &res_buffer.data()[index],50000,0);
  if(res_sz < 0){
    handle502(client_fd);
    logFile << id << ": ERROR 502 : proxy cannot get valid message from server" << std::endl;
  }

  index += res_sz;
  std::string re(res_buffer.begin(),res_buffer.end());
  cout<<"SUCCESS: get response from server, the response is:"<<endl<< re <<endl;
  Response response(re);
  int contentL=response.findContentL();
  cout<<"Response content length: "<< contentL <<endl;
  // log response
  // std::string firstLine=pap.findFirstLine();
  // LP.Receive(id,firstLine , hostname);
  pthread_mutex_lock(&mutex);
  logFile << id << ": Received \"" << response.line << "\" from " << host
          << std::endl;
  pthread_mutex_unlock(&mutex);

  int count=0;
  while((res_sz=recv(server_fd, &res_buffer.data()[index],50000,0))>0){
      count += res_sz;
      index += res_sz;
      if(count>=contentL){
          break;
      }         
      res_buffer.resize(res_buffer.size()+res_sz);
  }

  cout<<"The completed response is:"<<endl<< res_buffer.data()<<endl;

  // send response to client
  send(client_fd,res_buffer.data(),index,0);

  pthread_mutex_lock(&mutex);
  logFile << id << ": Responding \"" << response.line << "\"" << std::endl;
  pthread_mutex_unlock(&mutex);

  // close socket
  close(server_fd);
  return NULL;
}


void * proxy::handleGET(Request *req, client_info client, int client_fd, ssize_t len, int server_fd){
  std::string info = req->line;
  std::string host = req->host;
  std::string input = req->input;
  std::unordered_map<std::string, Response>::iterator it;
  int ID = client.getID();
  it = Cache.find(info);
  pthread_mutex_lock(&mutex);
	logFile<<ID<<": Requesting \""<<req->line<<"\" from " <<host<<std::endl;
	pthread_mutex_unlock(&mutex);

  if(it == Cache.end()){
    pthread_mutex_lock(&mutex);
    logFile<<client.getID()<<": not in cache"<<std::endl;
	  //  <<"Requesting \""<<info<<"\" from "<<host<<std::endl;
    pthread_mutex_unlock(&mutex);
    std::cout<<"the input of the res:"<<std::endl
	     <<input.c_str()<<std::endl;
    send(server_fd,(char*)input.c_str(),len,0);
    send_server(req,server_fd, client_fd,ID,req->host);
  }
  else{
    if((it->second).nocache_flag){
      bool flag = revalidate(&(it->second), req->input, server_fd, ID);
      if(flag == false){

	send(server_fd,(char*)input.c_str(),len,0);
	send_server(req,server_fd,client_fd,ID,host);
      }
      else{
	send_cacheinfo(it->second, client_fd, ID);
      }
    }
    else{
      bool check_valid =checkExp(server_fd,&(it->second),req,ID);
      if(check_valid){
	send_cacheinfo(it->second, client_fd, ID);
      }
      else{
	pthread_mutex_lock(&mutex);
	logFile<<ID<<": Requesting \""<<req->line<<"\" from "<<host<<std::endl;
	pthread_mutex_unlock(&mutex);
	send(server_fd,(char*)input.c_str(),len,0);
	send_server(req,server_fd,client_fd,ID,host);
      }
    }
    cacheprinter();
  }
  return NULL;
}
void proxy::send_cacheinfo(Response res, int client_fd, int ID){
  std::string clit_response = res.response;
  std::string clit_line = res.line;
  send(client_fd,&clit_response,clit_response.size(),0);
  pthread_mutex_lock(&mutex);
  logFile<< ID << ": Responding \""<<clit_line<<"\""<<std::endl;
  pthread_mutex_unlock(&mutex);
}

void proxy::send_server(Request* req,int server_fd, int client_fd, int ID, std::string host){
    std::vector<char> server_resp(65536);
    ssize_t resp_len = recv(server_fd, &(server_resp[0]),65535,0);
    if(resp_len <= 0){
      return;
    }
    size_t res_total = 0;
    res_total +=resp_len;
    std::vector<char> response ;
    std::vector<char>::iterator it = server_resp.begin() + resp_len;
    response.insert(response.begin(), server_resp.begin(), it);
    std::string str="";
    str.insert(str.begin(),response.begin(),response.end());
    Response *res = new Response(str);
    logFile << ID <<": Received \""<<res->line<<"\" from " <<req->host <<std::endl;
    if(res->isChunk()){
      pthread_mutex_lock(&mutex);
      logFile<< ID <<": is chunked thus not in cache"<<std::endl;
      pthread_mutex_unlock(&mutex);
      send(client_fd, server_resp.data(), resp_len, 0);
      while(resp_len > 0){
	resp_len = recv(server_fd, &(server_resp[0]),65535, 0);
	if(resp_len <= 0){
	  std::cout<< "chunked msg ends"<<std::endl;
	  break;
	}
	vector<char>::iterator it = server_resp.begin() + resp_len;
	response.insert(response.end(), server_resp.begin(), it);
	res_total += resp_len;
	send(client_fd, server_resp.data(), resp_len,0);
      }
    }
    else{
      bool no_store = false;
      if((res->response).find("no-store")!=std::string::npos){
	no_store = true;
      }
      int exp_length = res ->findContentL();
      if(exp_length!= -1){
	std::vector<char> new_res(65536);
	while(res_total < exp_length){
	  resp_len = recv(server_fd, &(new_res[0]), 65535,0);
	  if(resp_len <= 0){
	    break;
	  }
	  vector<char>::iterator it = new_res.begin() + resp_len;
	  response.insert(response.end(), new_res.begin(), it);
	  res_total += resp_len;
	}
	send(client_fd, response.data(),response.size(),0);
	res->updateres(response);
	//send response to the client
      }
      else{//if contentL == -1
	res->updateres(response);
	send(client_fd,response.data(),response.size(),0);
      }
	print_cacheinfo(res,no_store,req->line,ID,10);
    }
    std::string res_line = "";
    res_line.insert(res_line.begin(),response.begin(),response.end());
    size_t pos = res_line.find("\r\n");
    std::string subs = res_line.substr(0,pos);
    std::cout<<"Responding for Get request"<<std::endl;
    pthread_mutex_lock(&mutex);
    logFile<<ID<<": Responding \""<<res->line<<"\""<<std::endl;
    pthread_mutex_unlock(&mutex);
}

void proxy::print_cacheinfo(Response *res,bool no_store,std::string line,int ID,int max_capacity){
  if((res->response).find("HTTP/1.1 200 OK") != std::string::npos){
    if(no_store){
      pthread_mutex_lock(&mutex);
      logFile<<ID<<": not in cache but no-store"<<std::endl;
      pthread_mutex_unlock(&mutex);
    }
    else{
      pthread_mutex_lock(&mutex);
      logFile<<ID<<": stored in cache"<<std::endl;
      pthread_mutex_unlock(&mutex);
    }
    if(Cache.size()>max_capacity){
      std::unordered_map<std::string,Response>::iterator it = Cache.begin();
      Cache.erase(it);      
    }
    Cache.insert(std::pair<std::string,Response>(line,*res));
  }
}

bool proxy::revalidate(Response *res, std::string req, int server_fd, int ID){
  std::string new_req = req;
  std::string modify_pos = "Last_Modified: ";
  std::string new_res = res->response;
  size_t pos = new_res.find(modify_pos);
  if(pos != std::string::npos){
    time_t time = res->last_modified;
    std::string tm = ctime(&time);
    std::string toAdd = "If-Modified-Since: " + tm.append("\r\n");
    new_req = new_req.insert(new_req.length()-1,toAdd);
  }
  else if(res->Etag == ""){
    return true;
  }
  if(res->Etag != ""){
    std::string toAdd = "If-None-Match: " + res->Etag.append("\r\n");
    new_req = new_req.insert(new_req.length() - 2, toAdd);
  }
  char temp_msg[new_req.size() + 1];
  int sd_len = send(server_fd, temp_msg, new_req.size() + 1, 0);//change needed
  if(sd_len <= 0){
    std::cout<<"Revalidation: fail to send"<<std::endl;
  }
  else{
    std::cout<<"Revalidation: successful to send"<<std::endl;
  }
  vector<char> buffer(65536);
  int rec_len = recv(server_fd, &(buffer[0]), 65536,0);
  if(rec_len <= 0){
    std::cout<<"Revalidation: fail to receive"<<std::endl;
  }
  std::string new_rec = "";
  std::vector<char>::iterator it = buffer.begin() + rec_len;
  new_rec.insert(new_rec.begin(), buffer.begin(), it);
  if(new_rec.find("HTTP/1.1 200 OK")!= std::string::npos){
    pthread_mutex_lock(&mutex);
    logFile << ID <<": in cache, but is modified"<<std::endl;
    pthread_mutex_unlock(&mutex);
    return false;
  }
  //304
  else{
    pthread_mutex_lock(&mutex);
    logFile << ID <<": in cache and can be used"<<std::endl;
    pthread_mutex_unlock(&mutex);
    return true;
  }
}

bool proxy::checkExp(int server_fd, Response *res, Request *req, int ID){
  long fresh_time = res->findLifetime();
  time_t curr_time = time(nullptr);
  time_t res_time = res->date;
  time_t expi_time = res_time + fresh_time;
  char *tm = ctime(&expi_time);
  if((fresh_time!= -1) && (fresh_time != 0)){
    if(expi_time <= curr_time){
      //expired
      Cache.erase(req->line);
      pthread_mutex_lock(&mutex);
      logFile<< ID <<": in cache, already expired at "<<tm<<std::endl;
      pthread_mutex_unlock(&mutex);
      return false;
    }
    else{
      pthread_mutex_lock(&mutex);
      logFile <<ID<<": in cache, will expire at "<<tm<<std::endl;
      pthread_mutex_unlock(&mutex);
    }
  }
  bool check_val = revalidate(res,req->input,server_fd,ID);
  if(check_val == false){
    return false;
  }
  pthread_mutex_lock(&mutex);
  logFile<< ID <<": in cache and valid"<<std::endl;
  pthread_mutex_unlock(&mutex);
  return true;
}
void proxy::cacheprinter(){
  std::unordered_map<std::string, Response>::iterator it = Cache.begin();
  std::cout<<"--------Cache--------"<<std::endl;
  while(it != Cache.end()){
    std::cout<<it->first<<std::endl;
    std::cout<<(it->second).response.substr(0,300)<<std::endl;
    std::cout<<"---------------------"<<std::endl;
    ++it;
  }
  std::cout<<"------Cache size------"<<std::endl;
  std::cout<<"cache.size ="<<Cache.size()<<std::endl;
  std::cout<<"---------------------"<<std::endl;
}

void proxy::handle502(int client_fd){
  std::string msg502 = "HTTP/1.1 502 Bad Gateway";
  send(client_fd, msg502.c_str(), msg502.size(), 0);
}