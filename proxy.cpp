//Cody Pitts
//Jackson Van Dyck
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <ostream>
#include <pthread.h>
#include <vector>
#include <string>
#include <sstream>
#include <numeric>
#include <semaphore.h>
using namespace std;

#define BACKLOG 10 //how many pending connections queue will hold
#define MAXDATASIZE 2000000
const int MAXARGUMENTS = 1000;
sem_t mut;

pthread_mutex_t mutLock;
string absoluteToRelative(string absolute_uri, string &server_port_num, string &hostname);

string parseClientArguments(string unparsed_message,
                                    string delimiter, string &server_port_num, string &hostname);

void* threadFunc(void * t_args);

string parse();

//declare struct for the threads to use

struct thread_args
{
  int* thread_size_ptr;
  int* socketNum;
  int* comm_sock_num;
  struct addrinfo hints;

};

int main(int argc, char *argv[])
{
  //variables for server listening
  int listen_sock;
  //int comm_sock;  //listen_sock is listen, comm_sock is talk
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; //client's address info
  struct sigaction sa;
  socklen_t sin_size;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  char buffer[MAXDATASIZE];
  char* bp = buffer;
  int curr_sem_value;
  for(int i = 0; i < MAXDATASIZE; i++)
    buffer[i] = '\0';

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  //variables for threading  
  int num_threads = 30; 
  vector<pthread_t> thread_pool;
  thread_pool.resize(30);
  sem_init(&mut,0,-1);
  pthread_mutex_init(&mutLock, NULL);
  struct thread_args *t_args = new thread_args;
  t_args->comm_sock_num = new int;

  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  //loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((listen_sock = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
      continue;
    }
     if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {;
       exit(1);
     }

    if (bind(listen_sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(listen_sock);
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo); //no longer need this structure

  if (p == NULL)  {
    exit(1);
  }

  if (listen(listen_sock, BACKLOG) == -1) {
    exit(1);
  }

  t_args->hints = hints;

  for(int i = 0; i < num_threads; i++)
  {
    pthread_t temp_t;
    pthread_create(&temp_t, NULL, threadFunc, (void*) t_args);
    thread_pool.push_back(temp_t);
  }

  while(1) {
    sin_size = sizeof their_addr; 
    sem_getvalue(&mut,&curr_sem_value);
    *(t_args->comm_sock_num) = accept(listen_sock, (sockaddr *)&their_addr, &sin_size);
    if (*(t_args->comm_sock_num) == -1) {
      continue;
    }
    sem_post(&mut);
  }
  delete t_args;

  return 0;
  pthread_exit(NULL);
}

void* threadFunc(void* t_args)
{
  struct thread_args* passed_args = (struct thread_args*) t_args;
  string unparsed_message;
  struct addrinfo *thread_info, *p;
  string server_port_num;
  int server_sock;
  string hostname;
  int byte_read;
  int byte_sent;
  int rv;
  char buffer[MAXDATASIZE];
  char* bp = buffer;
  string carriageRet = "\r\n";
  string temp_len_1, temp_len_2;
  string receivedData;
  string tempData;
  int oldDataLen = -1;
  bool read = true;
  bool test= true;
  int bytes_read;
  string errorNum = "";

  sem_wait(&mut);

  pthread_mutex_lock(&mutLock);
  int thread_sock = *(*passed_args).comm_sock_num;

  pthread_mutex_unlock(&mutLock);
  for(int i = 0; i < MAXDATASIZE; i++)
    buffer[i] = '\0';

  while(read)
  {
    while (( bytes_read = recv(thread_sock, (void*)bp, MAXDATASIZE, 0)) > 0)
    {
      if( *(bp + bytes_read) == '\0')
        break;
      bp += bytes_read;
      if(bytes_read == 0)
        break;
    }
    receivedData += string(bp);
    if(receivedData.find(carriageRet+carriageRet) != -1 || receivedData.length() == oldDataLen)
    {
      read = false;
    }
    else
    {
      //clear the buffer
      for(int i = 0; i < MAXDATASIZE; i++)
        buffer[i] = '\0';
      oldDataLen = receivedData.length();
    }
  }

	string parsed_input = parseClientArguments(receivedData, carriageRet, server_port_num, hostname);

  if(parsed_input == "Internal Error"){
    errorNum =  "500 'Internal Error'";
    do{
      byte_sent = send(thread_sock, (void*) errorNum.c_str(), errorNum.length(), 0);
    }while(byte_sent > 0 && byte_sent != (int) errorNum.length());
    return NULL;
  }
 
  //now we need to send and receive to server
  if((rv = getaddrinfo(hostname.c_str(), server_port_num.c_str(), &(*passed_args).hints, 
    &thread_info)) != 0){  
    errorNum = "Error 500";
    do{
      byte_sent = send(thread_sock, (void*) errorNum.c_str(), errorNum.length(), 0);
    }while(byte_sent > 0 && byte_sent != (int) errorNum.length());
    return NULL;
  }

  for(p = thread_info; p != NULL; p = p->ai_next) {
    if ((server_sock = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
      continue;
    }

    if (connect(server_sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_sock);
      continue;
    }
    break;
  }

  if (p == NULL) {
    errorNum = "500 'Internal Error'";
    do{
      byte_sent = send(thread_sock, (void*) errorNum.c_str(),errorNum.length(), 0);
    }while(byte_sent > 0 && byte_sent != (int)errorNum.length());
    return NULL;
  }

  do{
    byte_sent = send(server_sock, (void*) parsed_input.c_str(), parsed_input.length(), 0);  
  }while(byte_sent > 0 && byte_sent != (int) parsed_input.length());
 

  read = true;
  receivedData = "";
  char receive[MAXDATASIZE];
  char* recPtr = receive;
  while(read)
  {
    while ((bytes_read = recv(server_sock, (void*)recPtr, MAXDATASIZE, 0)) > 0)
    {
      if( *(bp + bytes_read) == '\0')
        break;
      bp += bytes_read;
      if(bytes_read == 0)
        break;
    }

    receivedData += string(recPtr);
    if(receivedData.length() == oldDataLen)
    {
      read = false;
    }
    else
    {
      //clear the buffer
      for(int i = 0; i < MAXDATASIZE; i++)
        receive[i] = '\0';
      oldDataLen = receivedData.length();
    }
  }
  int data_sent = 0;
  do{
    byte_sent = send(thread_sock, (void*) receivedData.c_str(), receivedData.length(), 0);
    data_sent += byte_sent;
  }while(byte_sent > 0 && data_sent < (int) receivedData.length());


  close(thread_sock);
  return NULL;
}



string absoluteToRelative(string absolute_uri, string &server_port_num, string &hostname){

  stringstream ssin(absolute_uri);
  int count = 0;
  string chunks[3];
  string unparsed_url;
  string temp_url;
  string path = "/";
  string complete;
  size_t host_start_ind;
  size_t host_end_ind;
  size_t port_pos;
  string temp_port;

  while(ssin){
    ssin >> chunks[count];
    ++count;
  }

  unparsed_url = chunks[1];

  host_start_ind = unparsed_url.find("www");
  if(host_start_ind >= 100)
    host_start_ind = unparsed_url.find("http://") + 7;
  temp_url = unparsed_url.substr(host_start_ind);

  host_end_ind = temp_url.find("/");
  path += temp_url.substr(host_end_ind + 1);
  hostname = temp_url.substr(0, host_end_ind);

  port_pos = hostname.find(":");

  if(port_pos != string::npos){ 
    temp_port = hostname.substr(port_pos + 1);
    server_port_num = temp_port;
  }
  else{
    server_port_num = "80";
  }

  if(chunks[0].find("GET") == string::npos)
    return "Internal Error";

  complete = std::string(chunks[0]) + " " + path + " " + chunks[2] + "\r\n";

  return complete;
}

string parseClientArguments(string unparsed_message,
                                    string delimiter, string &server_port_num, string &hostname){

  size_t index = 0;
  string temp;
  vector<string> arg_lines;
  arg_lines.resize(MAXARGUMENTS);
  bool edited_connection = false;
  bool edited_hostname_header = false;
  string finished_request;
  int count = 0;
  while((index = unparsed_message.find(delimiter)) != string::npos){
    temp = unparsed_message.substr(0, index + delimiter.length());
    arg_lines[count] = temp;
    count++;
    unparsed_message.erase(0, index + delimiter.length());
  }

  arg_lines[0] = absoluteToRelative(arg_lines[0], server_port_num, hostname);

  if (arg_lines[0] == "Internal Error"){
    return "Internal Error";
  }
  for(unsigned int i = 0; i < arg_lines.size(); i++){
    if(arg_lines[i].find("Connection:") != string::npos){
      arg_lines[i] = "Connection: close\r\n";
      edited_connection = true;
    }
  }

  if(edited_connection == false)
    arg_lines[1] += "Connection: close\r\n";

  for(unsigned int i = 0; i < arg_lines.size(); i++){
    if(arg_lines[i].find("Host:") != string::npos){
      arg_lines[i] = "Host: " + hostname + "\r\n";
      edited_hostname_header = true;
    }
  }

  if(edited_hostname_header == false)
    arg_lines[1] = "Host: " + hostname + "\r\n" + arg_lines[1];

  finished_request = accumulate(arg_lines.begin(), arg_lines.end(), string(""));

  return finished_request;
}





