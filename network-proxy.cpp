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
using namespace std;

const int MAXDATASIZE = 1000;

string constructMessage(string method_str, string path_str,
                       string http_ver_str, string hostname_str,
                       string header_str){
  string temp_str = "";

  temp_str += method_str;
  temp_str += " ";
  temp_str += path_str;
  temp_str += " ";
  temp_str += http_ver_str;
  temp_str += "\r\n";
  temp_str += hostname_str;
  temp_str += "\r\n";
  temp_str += header_str;
  temp_str += "Connection: close\r\n\r\n";

  return temp_str;
  }

int main(int argc, char *argv[])
{
  string method_str;
  string unparsed_url_str;
  string temp_url_str;
  string http_ver_str;
  string header_str = "";
  string hostname_str;
  string path_str = "/";
  size_t host_start_ind;
  size_t host_end_ind;
  //int message_len;
  string ready_message_str;

  method_str = argv[1];
  unparsed_url_str = argv[2];
  http_ver_str = argv[3];

  if(argc > 4){
    header_str = argv[4];
    header_str += ' ';
    header_str += argv[5];
    header_str += "\r\n";

    if(argc > 6){
      for(int i = 6; i < argc; i+=2){
        header_str += argv[i];
        header_str += ' ';
        header_str += argv[i+1];
        header_str += "\r\n";
      }
    }
  }

  host_start_ind = unparsed_url_str.find("www");
  temp_url_str = unparsed_url_str.substr(host_start_ind);
  host_end_ind = temp_url_str.find("/");
  path_str += temp_url_str.substr(host_end_ind + 1);
  hostname_str = temp_url_str.substr(0, host_end_ind);

  ready_message_str = constructMessage(method_str, path_str, http_ver_str,
                                   hostname_str, header_str);

  cout << ready_message_str;
}
















