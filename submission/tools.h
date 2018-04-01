#ifndef TOOLS_H
#define TOOLS_H

#define ERR 0
#define GET 1
#define POST 2

#define BUF_SIZE 1024

#define HEADER_LIST_INCREASE_FACTOR 5 // HTML Message의 header list 동적 할당 size는 5씩 증가한다.

struct URL
{
	char *ip;
	char *port;
	char *path;
};

struct HttpHeader
{
	char *key;
	char *value;
};

struct HttpHeaderList
{
	struct HttpHeader **headers;
	int length;
	int max_length;
};

struct HttpRequestMessage
{
	int method;
	char *ip;
	char *port;
	char *path;
	char *version;
	int contentLength;
	int headerLength;
	struct HttpHeaderList *hList; // 헤더 list를 저장하는 배열
	char *body; // 첫 번째 헤더 메세지 fragment에서 body의 시작 포인터를 가진다.
};

struct HttpResponseMessage
{
	char *version;
	char *status;
	struct HttpHeaderList *hList;
	int contentLength;
	char *body;
};

char* str_alloc(int length);
char* makeString(char str[]); // str을 문자열로 가지는 char*를 allocation해서 return
char* copyString(char *str);
void substr(char *dest, char *start, char *end);
void clearString(char *str);

char* makeHost(struct URL *url); // URL을 받아 ip:port 형식의 host를 만들어준다.

struct HttpHeader* createHeader(char *key, char *value);
struct HttpHeaderList* createHeaderList();
struct HttpRequestMessage* createRequestMessage(int method, struct URL *url, char *version, char *body, int contentLength);
struct HttpResponseMessage* createResponseMessage(char *version, char *status, int CL);
char* makeRequestHeaderString(struct HttpRequestMessage *target); // RequestMessage를 byte-string으로 변환해준다.
char* makeResponseHeaderString(struct HttpResponseMessage *target);

struct HttpRequestMessage* parseRequestMessage(char *message); // String을 파싱해 HttpRequestMessage로 만들어준다.

void addHeader(struct HttpHeaderList* hList, char *key, char *value);
struct HttpHeader* getHeader(struct HttpHeaderList* hList, int index);

void freeURL(struct URL *url);
void freeHeader(struct HttpHeader* target); // HttpHeader를 free한다. 이 때 key와 value pointer도 free하므로 매우 주의해야 한다.
void freeHeaderList(struct HttpHeaderList* target);
void freeRequestMessage(struct HttpRequestMessage* target);
void freeResponseMessage(struct HttpResponseMessage* target);

void printHeader(struct HttpHeader *target); // target을 print 합니다. (debugging function)
void printHeaderList(struct HttpHeaderList *target, char *comment);
void printRequestMessage(struct HttpRequestMessage *target, char *comment);

#endif