Name : SangHyeon Lee
Student ID : 20150548
Products : server, client (executable file)
Src Files : server_main.c/h, client_main.c/h, tools.c/h, Makefile

How to use:
./server -p [port]
./client -g [ip]:[port]/[filename]
./client -p [ip]:[port]/[filename] < [standard input]

These programs are simple HTTP(Hyper-Text Transfer Protocol) server and client. It provides simple POST and GET Method transfering. It can communicate HTTP Message less than 1GB. If you try to access nonexistent file, it sends 404 Not Found Error message to you. It sends also 400 Bad Request message when your http request is wrong.

Explanation about source

server_main.c/h : Server soruce code and header file. It parses command line arguments to its execution port. Next, If you start server, it is ready for http request message from a client. If the client sends http request message, server receives it, and parses the request. If the client sends POST, it saves payload to file which has given name from client. Also, if the client sends GET, it sends a file which is requested by the client.

client_main.c/h : Client source cdoe and header file. It parses command line arguments to its destination. Next, If you enter POST method, it reads a file which you want to upload. Therefore, it send HTTP Request Message with setupHttpServer and httpRequest function. Finally, it receives http response message from server.

tools.c/h : It is preset for HTTP Request and Response. It provides struct HttpRequestMessage, HttpResponseMessage, HttpServer, and more. Also, it includes various functions for free or allocation memory and structure.

for example:
struct HttpRequestMessage
{
    int method;
    char *ip;
    char *port;
    char *path;
    char *version;
    int contentLength;
    int headerLength;
    struct HttpHeaderList *hList;
    char *body;
};
Above is a custom struct for saving Http Request Message. If you want to make http request message, you call struct HttpRequestMessage* createRequestMessage(int method, struct URL *url, char *version, char *body, int contentLength) function. Also, you can parse plain text to http request message with struct HttpRequestMessage* parseRequestMessage(char *message) function.

struct HttpHeaderList
{
    struct HttpHeader **headers;
    int length;
    int max_length;
};
This is a data structure for saving http headers. It is a simple list contains HTTP Header such as ‘Content-Length’ or ‘Host’. If you want to add header in this list, you just call void addHeader(struct HttpHeaderList* hList, char *key, char *value). Also, you can get a header item with struct HttpHeader* getHeader(struct HttpHeaderList* hList, int index) function.