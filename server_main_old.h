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


#define RECV_SIZE 5000
#define FILE_SIZE 1024 * 1024 * 1024 + 3000

struct HttpServer
{
	int sock_server;
	int sock_client;
	struct sockaddr_in addr_server;
	struct sockaddr_in addr_client;
	fd_set fd_sockets;
	fd_set iter;
};

/*
int parseArguments(int argc, char *argv[], char **URL);
struct URL* parseURL(char *url_string);
char* readBody();
int httpRequest(struct HttpRequestMessage *message);*/
int parseArguments(int argc, char *argv[], short *port);
int startServer(short port);
struct HttpServer* setupHttpServer(short port);
int GetMethodProcess(struct HttpServer *http, struct HttpRequestMessage *req);
int PostMethodProcess(struct HttpServer *http, struct HttpRequestMessage *req);
int isFileExists(char *filename);
int sendFile(char *filename, struct HttpServer *http, struct HttpRequestMessage *req);

#endif