#ifndef CLIENT_MAIN_H
#define CLIENT_MAIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "tools.h"


#define BUF_SIZE 1024


int parseArguments(int argc, char *argv[], char **URL);
struct URL* parseURL(char *url_string);
char* readBody(int *length);
int httpRequest(struct HttpRequestMessage *message);

#endif