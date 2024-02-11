/* 
 * tcpserver.c - A multithreaded TCP echo server 
 * usage: tcpserver <port>
 * 
 * Testing : 
 * nc localhost <port> < input.txt
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>

using namespace std;

int processClientRequest(int &);
int createServerSocket(const int &);

int main(int argc, char ** argv) {
  int portno; /* port to listen on */
  
  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // DONE: Server port number taken as command line argument
  portno = atoi(argv[1]);

  // DONE: Create a server socket
  int serverSocket = createServerSocket(portno);

  if (listen(serverSocket, 5) < 0)
  {
    cerr << "Error: Unable to listen on socket" << endl;
    close(serverSocket);
    return -1;
  }

  cout << "Server listening on port: " << portno << endl;

}

int createServerSocket(const int & portno) {
  int sockfd; /* socket */
  struct sockaddr_in serveraddr; /* server's addr */
  
  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }
  
  /* 
   * build the server's Internet address 
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);
  
  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }
  
  return sockfd;
}