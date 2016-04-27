//Cody Pitts

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
#include <pthread.h>
#include <vector>
#include <string>
#include <sstream>
#include <numeric>
using namespace std;

#define BACKLOG 10 //how many pending connections queue will hold

#define MAXDATASIZE 1000
const int MAXARGUMENTS = 1000;

string absoluteToRelative(string absolute_uri, int &server_port_num);

string parseClientArguments(string unparsed_message,
                                    string delimiter, int &server_port_num);

//some functionality from Beej
void sigchld_handler(int s);

//get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

void* threadFunc(string unparsed_message);

string parse();

int main(int argc, char *argv[])
{
  //variables for server listening
  int listen_sock, comm_sock;  //listen_sock is listen, comm_sock is talk
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; //client's address info
  struct sigaction sa;
  socklen_t sin_size;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  char buffer[MAXDATASIZE];
  char* bp = buffer;

  for(int i = 0; i < MAXDATASIZE; i++)
    buffer[i] = '\0';

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  //variables for threading  
  int current_size = 0;
  int num_threads = 30;
  pthread_mutex_t mut;
  pthread_mutex_init(&mut, NULL);
  //pool of current threads
  //vector<pthread_t> threads;
  bool running;
  pthread_mutex_t condition;
  pthread_t thread_pool[num_threads];



  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  //loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((listen_sock = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
      cerr << "Socket";
      continue;

    }

    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1) {
      cerr << "Setsockopt";
      exit(1);
    }

    if (bind(listen_sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(listen_sock);
      cerr << "Bind";
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); //no longer need this structure

  if (p == NULL)  {
    fprintf(stderr, "Failed to bind\n");
    exit(1);

  }

  if (listen(listen_sock, BACKLOG) == -1) {
    cerr << "Listen";
    exit(1);
  }

  sa.sa_handler = sigchld_handler; //reap all dead processes (Beej)
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    cerr << "Sigaction";
    exit(1);
  }

  printf("Waiting for connections...\n");


  while(1) {
    sin_size = sizeof their_addr;
    comm_sock = accept(listen_sock, (struct sockaddr *)&their_addr, &sin_size);
    if (comm_sock == -1) {
      cerr << "Accepting";
      continue;
    }

    //start thread stuff

	//DO WE NEED TO LOCK HERE?
	//if thread pool is full
	if(current_size > 30)
		return 0;
		//unlock as well if we need lock
	string current_input;
	//create a thread with a function to parse, send, and receive shit
	pthread_t current_thread = thread_pool[current_size];
	pthread_create(current_thread, NULL,
				   threadFunc, (void*)comm_sock);
	current_size++;
	//unlock here if lock is needed


    /*
    if (!fork()) { //in child now
      //runFinger(comm_sock, bp);

      close(listen_sock); //child doesn't need the listen socket

      close(comm_sock);
      exit(0);
    }
    */
    close(comm_sock);  //parent doesn't need this
  }

  return 0;
}




void sigchld_handler(int s)
{
  //waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while(waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}

//get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* threadFunc(string unparsed_message, int &server_port_num)
{
	string parsed_input = parseClientArguments(unparsed_message, "\r\n", server_port_num);
	string ret;
	//now we need to send and receive
}

string absoluteToRelative(string absolute_uri, int &server_port_num){

  stringstream ssin(absolute_uri);
  int count = 0;
  string chunks[3];
  string unparsed_url;
  string temp_url;
  string hostname;
  string path = "/";
  string complete;
  size_t host_start_ind;
  size_t host_end_ind;
  size_t port_pos;
  string temp_port;

  while(ssin){
    if(count > 3)
      cerr << "Malformed request";

    ssin >> chunks[count];
    ++count;
  }

  unparsed_url = chunks[1];
  host_start_ind = unparsed_url.find("www");
  temp_url = unparsed_url.substr(host_start_ind);
  host_end_ind = temp_url.find("/");
  path += temp_url.substr(host_end_ind + 1);
  hostname = temp_url.substr(0, host_end_ind);

  port_pos = hostname.find(":");

  if(port_pos != string::npos){
    temp_port = hostname.substr(port_pos + 1);
    server_port_num = atoi(temp_port.c_str());
  }

  complete = std::string(chunks[0]) + " " + path + " " + chunks[2] + "\r\n" +
    + "Host: " + hostname + "\r\n";

  return complete;
}

string parseClientArguments(string unparsed_message,
                                    string delimiter, int &server_port_num){

  size_t index = 0;
  string temp;
  vector<string> arg_lines;
  bool edited_connection = false;
  string finished_request;

  while((index = unparsed_message.find(delimiter)) != string::npos){
    temp = unparsed_message.substr(0, index + delimiter.length());
      arg_lines.push_back(temp);
      unparsed_message.erase(0, index + delimiter.length());
  }

  arg_lines[0] = absoluteToRelative(arg_lines[0], server_port_num);

  for(unsigned int i = 0; i < arg_lines.size(); i++){
    if(arg_lines[i].find("Connection:") != string::npos){
      arg_lines[i] = "Connection: close\r\n";
      edited_connection = true;
    }
  }

  if(edited_connection == false)
    arg_lines[1] += "Connection: close\r\n";

  finished_request = accumulate(test_vec.begin(), test_vec.end(), string(""));

  return finished_request;

}





