#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include "tools.h"


#define BUF_SIZE 1024

struct HttpServer
{
	int sock_server;
	int sock_client;
	struct sockaddr_in addr_server;
	struct sockaddr_in addr_client;
	fd_set fd_sockets;
	fd_set iter;
};


int isFileExists(char *filename);
int getFileSize(char *filename);
int parseArguments(int argc, char *argv[], short *port);
struct HttpServer* setupHttpServer(short port);
int startServer(short port);


#endif