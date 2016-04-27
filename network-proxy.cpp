#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <numeric>
using namespace std;

const int MAXDATASIZE = 1000;
const int MAXARGUMENTS = 100;

string receiveClientMessage(int socket){
  int byte_read = 0;
  char *message_buffer = new char[MAXDATASIZE + 1];
  string temp;

  for(int i = 0; i < MAXDATASIZE + 1; i++)
    message_buffer[i] = '\0';

  char *buf_ptr = message_buffer;

  while((byte_read = recv(socket, (void*)buf_ptr, MAXDATASIZE, 0)) > 0){
    if(*(buf_ptr + byte_read) == '\0')
      break;
    buf_ptr += byte_read;
  }

  temp = string(buf_ptr);
  delete[] message_buffer;
  return temp;
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

vector<string> splitClientArguments(string unparsed_message,
                                    string delimiter, int &server_port_num){
  size_t index = 0;
  string temp;
  vector<string> arg_lines;
  bool edited_connection = false;

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

  return arg_lines;
}

int main(int argc, char *argv[])
{
  string method_str;
  string unparsed_url_str;
  string http_ver_str;
  string header_str = "";
  string hostname_str;
  string path_str = "/";
  //int message_len;
  string ready_message_str;
  vector<string> test_vec;
  int server_port_num;

  string test = "GET http://www.seattleu.edu:99/index.html HTTP/1.0\r\nApples: \
oranges\r\n\r\n";
  string result;

  test_vec = splitClientArguments(test, "\r\n", server_port_num);

  result = accumulate(test_vec.begin(), test_vec.end(), string(""));

  cout << "Server port number: " << server_port_num << endl;
  cout << result;

}