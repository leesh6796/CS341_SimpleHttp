#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tools.h"

char* str_alloc(int length)
{
    //return (char*)malloc(length + 1);
    return (char*)calloc(length + 1, sizeof(char));
}

char* makeString(char str[]) // str을 문자열로 가지는 char*를 allocation해서 return
{
    int len = strlen(str);
    char *new_string = str_alloc(len);
    strncpy(new_string, str, len);
    new_string[len] = '\0';

    return new_string;
}

char* copyString(char *str) // 같은 내용을 가지는 다른 string 포인터를 allocation해서 return
{
    return makeString(str);
}

// Python의 Slicing과 그 기능이 같다.
void substr(char *dest, char *start, char *end)
{
    int length = end - start;
    strncpy(dest, start, length);
    dest[length] = '\0';
}


// 해당 문자열을 전부 cleaning 한다.
void clearString(char *str)
{
    int length = strlen(str);
    memset(str, 0, length);
}

char *makeHost(struct URL *url) // IP:port 형태로 만들어준다.
{
    int len_ip = strlen(url->ip);
    int len_port = strlen(url->port);

    int len_host = len_ip + 1 + len_port; // 1은 콜론
    char *host = str_alloc(len_host);
    snprintf(host, len_host + 1, "%s:%s", url->ip, url->port); // snprintf의 n parameter는 끝의 널 문자를 포함한 개수이다.

    return host;
}

struct HttpHeader* createHeader(char *key, char *value)
{
    struct HttpHeader *new_header = (struct HttpHeader*)malloc(sizeof(struct HttpHeader));

    new_header->key = key;
    new_header->value = value;

    return new_header;
}

struct HttpHeaderList* createHeaderList()
{
    struct HttpHeaderList *new_list = (struct HttpHeaderList*)malloc(sizeof(struct HttpHeaderList));

    new_list->headers = (struct HttpHeader**)malloc(sizeof(struct HttpHeader*) * HEADER_LIST_INCREASE_FACTOR); // 처음에 5개까지 들어갈 수 있는 리스트를 만든다.
    new_list->length = 0;
    new_list->max_length = HEADER_LIST_INCREASE_FACTOR;

    return new_list;
}

struct HttpRequestMessage* createRequestMessage(int method, struct URL *url, char *version, char *body, int contentLength)
{
    struct HttpRequestMessage *new_message = (struct HttpRequestMessage*)malloc(sizeof(struct HttpRequestMessage));

    new_message->method = method;
    new_message->ip = copyString(url->ip);
    new_message->port = copyString(url->port);
    new_message->path = copyString(url->path);
    new_message->version = copyString(version);
    new_message->contentLength = contentLength;
    new_message->headerLength = 0;
    new_message->hList = createHeaderList();
    new_message->body = body; // body는 크기가 크므로 copy하면 성능 저하가 일어난다. 따라서, Caller에서 free해주도록 한다.

    char *CL = NULL;
    if(method == GET || body == NULL) CL = makeString("0");
    else
    {
        int bodyLength = new_message->contentLength;
        if(bodyLength == 0) CL = makeString("0");
        else
        {
            int stringSizeCL = log10(bodyLength) + 1; // content-length의 string size를 구한다.
            CL = str_alloc(stringSizeCL);
            snprintf(CL, stringSizeCL + 1, "%d", bodyLength); // snprintf의 n은 \0 포함한 길이다.
        }
    }

    addHeader(new_message->hList, makeString("host"), makeHost(url)); // header에 들어가는 string은 모두 freeHeader에서 free한다. (성능상의 이유로)
    addHeader(new_message->hList, makeString("Content-Length"), CL); // contentLength string도 freeHeader할 때 free된다.

    return new_message;
}


struct HttpResponseMessage* createResponseMessage(char *version, char *status, int CL)
{
    struct HttpResponseMessage *new_message = (struct HttpResponseMessage*)malloc(sizeof(struct HttpResponseMessage));

    new_message->version = copyString(version);
    new_message->status = copyString(status);
    new_message->contentLength = CL;
    new_message->hList = createHeaderList();
    new_message->body = NULL; // body는 크기가 크므로 copy하면 성능 저하가 일어난다. 따라서, Caller에서 free해주도록 한다.

    char *contentLength = NULL;
    int bodyLength = new_message->contentLength;
    if(bodyLength == 0) contentLength = makeString("0");
    else
    {
        int stringSizeCL = log10(bodyLength) + 1; // content-length의 string size를 구한다.
        contentLength = str_alloc(stringSizeCL);
        snprintf(contentLength, stringSizeCL + 1, "%d", bodyLength); // snprintf의 n은 \0 포함한 길이다.
    }
    
    addHeader(new_message->hList, makeString("Content-Length"), contentLength);

    return new_message;
}


char* makeRequestHeaderString(struct HttpRequestMessage *target)
{
    // message의 길이를 계산하는 과정
    int len_method = 5; // GET or POST는 최대 4글자 + 공백
    int len_path = strlen(target->path);
    int len_version = 1 + 5 + strlen(target->version) + 4; // 공백 + HTTP/VERSION\r\n, 4는 \r\n
    int len_header = 0;

    int i;
    for(i=0; i<target->hList->length; i++)
    {
        struct HttpHeader *iter = getHeader(target->hList, i);
        len_header += strlen(iter->key) + 2 + strlen(iter->value) + 4; // key: value\r\n. 2는 :공백, 4는 \r\n
    }

    len_header += 4; // 마지막 줄의 \r\n

    int MessageLength = len_method + len_path + len_version + len_header + target->contentLength + 1;
    char *message = str_alloc(MessageLength);
    int cursor = 0;

    // METHOD
    if(target->method == GET)
    {
        sprintf(message, "GET ");
        cursor = 4;
    }
    else
    {
        sprintf(message, "POST ");
        cursor = 5;
    }

    // Path, version
    sprintf(message + cursor, "%s HTTP/%s\r\n", target->path, target->version);
    cursor += strlen(message + cursor);

    // Header
    for(i=0; i<target->hList->length; i++)
    {
        struct HttpHeader *iter = getHeader(target->hList, i);
        sprintf(message + cursor, "%s: %s\r\n", iter->key, iter->value);
        cursor += strlen(message + cursor);
    }

    // End of Header
    sprintf(message + cursor, "\r\n");
    cursor += strlen(message + cursor);

    /*if(target->contentLength > 0)
    {
        // Body
        sprintf(message + cursor, "%s", target->body);
        cursor += strlen(message + cursor);
    }*/

    *(message + cursor) = '\0';
    *(message + cursor + 1) = '\0';

    return message;
}


char *makeResponseHeaderString(struct HttpResponseMessage *target)
{
    int len_version = 5 + strlen(target->version) + 1; // 1은 공백
    int len_status = strlen(target->status) + 1 + 4; // 1은 공백, 4는 \r\n
    int len_header = 0;

    int i;
    for(i=0; i<target->hList->length; i++)
    {
        struct HttpHeader *iter = getHeader(target->hList, i);
        len_header += strlen(iter->key) + 2 + strlen(iter->value) + 4; // key: value\r\n. 2는 :공백, 4는 \r\n
    }

    len_header += 4; // 마지막 줄의 \r\n

    int MessageLength = len_version + len_status + len_header + target->contentLength + 1;
    char *message = str_alloc(MessageLength);
    int cursor = 0;

    // Path, version
    sprintf(message + cursor, "%s %s\r\n", target->version, target->status);
    cursor += strlen(message + cursor);

    // Header
    for(i=0; i<target->hList->length; i++)
    {
        struct HttpHeader *iter = getHeader(target->hList, i);
        sprintf(message + cursor, "%s: %s\r\n", iter->key, iter->value);
        cursor += strlen(message + cursor);
    }

    // End of Header
    sprintf(message + cursor, "\r\n");
    cursor += strlen(message + cursor);

    *(message + cursor) = '\0';
    *(message + cursor + 1) = '\0';

    return message;
}


struct HttpRequestMessage* parseRequestMessage(char *message)
{
    struct HttpRequestMessage *new_message = (struct HttpRequestMessage*)malloc(sizeof(struct HttpRequestMessage));
    memset(new_message, 0, sizeof(struct HttpRequestMessage));
    new_message->hList = createHeaderList();

    char *ptr;
    char *next_ptr;
    char *start, *end;

    int line = 0;
    int end_of_header = 0;
    int isThereCL = 0; // is There Content-Length?

    char *real_end; // 실제 메세지 끝

    char *buf = str_alloc(100);

    // 처음에는 left, right의 위치가 같다.
    start = message;
    end = message;
    real_end = message + BUF_SIZE;

    while((end = strchr(start, '\n')))
    {
        if(!end_of_header && end - start == 1) // \r\n이면
        {
            end_of_header = 1; // 헤더가 끝났다.
            start = end + 1;

            new_message->body = end + 1; // body
            new_message->headerLength = new_message->body - message;

            break;
        }

        if(!end_of_header) // 헤더를 파싱할 때는
        {
            substr(buf, start, end - 1); // -1은 \r

            if(line == 0) // 첫 번째 줄이면, METHOD PATH VERSION
            {
                if((ptr = strtok_r(buf, " ", &next_ptr)) == NULL) return NULL; // METHOD
                else
                {
                    if(!strcmp(ptr, "GET")) new_message->method = GET;
                    else if(!strcmp(ptr, "POST")) new_message->method = POST;
                    else return NULL;
                }

                if((ptr = strtok_r(NULL, " ", &next_ptr)) == NULL) return NULL; // PATH
                else new_message->path = copyString(ptr);

                if((ptr = strtok_r(NULL, " ", &next_ptr)) == NULL) return NULL; // VERSION
                else
                {
                    char *ans = makeString("HTTP/1.0");

                    if(strlen(ptr) != strlen(ans))
                    {
                        //printf("길이 다르다. %d %d\n%d", strlen(ptr), strlen(ans), ptr[8]);
                        return NULL;
                    }

                    if(strncmp(ptr, ans, strlen(ans))) // 1.0이 아니면
                    {
                        //printf("1.0이 아니다.\n");
                        return NULL;
                    }

                    new_message->version = copyString(ptr);
                }
            }
            else // 헤더면 key: value
            {
                char *middle = strchr(buf, ':') + 2; // value의 시작 char ptr
                if(middle == NULL) return NULL;
                
                char *key = str_alloc(middle - 2 - buf);
                substr(key, buf, middle - 2); // key를 parsing

                char *value = str_alloc(end - 1 - middle); // -1은 \r
                substr(value, middle, end - 1); // -1은 \r

                addHeader(new_message->hList, key, value);

                if(!strcmp(key, "Content-Length"))
                {
                    isThereCL = 1;
                    new_message->contentLength = atoi(value);
                    //new_message->body = str_alloc(new_message->contentLength + 1);

                    /*if(strlen(new_message->body) >= 0 && new_message->contentLength != strlen(new_message->body))
                    {길이 다른건 뒤에서 검사하자
                        //printf("길이가 다르다\n");
                        return NULL;
                    }*/
                }
            }
        }

        if(end + 1 <= real_end)
        {
            start = end + 1;
        }

        line++;
    }

    if(new_message->method == POST)
    {
        if(!isThereCL)
        {
            //printf("CL이 없다\n");
            return NULL;
        }

        int len_header = start - message;
        new_message->body = start;
    }

    return new_message;
}


void addHeader(struct HttpHeaderList* hList, char *key, char *value) // header는 freeHeader에서 key, value를 free하므로 주의해야한다.
{
    if(hList->max_length - 1 == hList->length) // 할당한 리스트만큼 헤더가 꽉 찼으면
    {
        hList->max_length += HEADER_LIST_INCREASE_FACTOR; // 헤더 리스트를 확장한다.
        hList->headers = (struct HttpHeader**)realloc(hList->headers, hList->max_length);
    }

    struct HttpHeader *item = createHeader(key, value);

    hList->headers[hList->length] = item;
    hList->length++;
}

struct HttpHeader* getHeader(struct HttpHeaderList* hList, int index)
{
    return hList->headers[index];
}

void freeURL(struct URL *url)
{
    free(url->ip);
    free(url->path);
    free(url);
}

void freeHeader(struct HttpHeader* target)
{
    free(target->key);
    free(target->value);
    free(target);
}

void freeHeaderList(struct HttpHeaderList* target)
{
    int i;

    for(i=0; i<target->length; i++)
    {
        freeHeader(getHeader(target, i));
    }

    free(target->headers);
    free(target);
}

void freeRequestMessage(struct HttpRequestMessage* target) // body는 free하지 않는다.
{
    free(target->ip);
    free(target->port);
    free(target->path);
    free(target->version);
    freeHeaderList(target->hList);
}

void freeResponseMessage(struct HttpResponseMessage* target)
{
    free(target->version);
    free(target->status);
    freeHeaderList(target->hList);
}

void printHeader(struct HttpHeader *target)
{
    printf("[HttpHeader] %s: %s\n", target->key, target->value);
}

void printHeaderList(struct HttpHeaderList *target, char *comment)
{
    int i;

    printf("[HttpHeaderList] ");
    if(comment != NULL) printf("%s", comment);
    printf("\n");

    for(i=0; i<target->length; i++)
    {
        printHeader(getHeader(target, i));
    }
}

void printRequestMessage(struct HttpRequestMessage *target, char *comment)
{
    printf("[HttpRequestMessage] ");
    if(comment != NULL) printf("%s", comment);
    printf("\n");

    if(target->method == GET) printf("GET ");
    else printf("POST ");

    printf("%s HTTP/%s\n", target->path, target->version);
    printHeaderList(target->hList, comment);

    printf("\n");

    if(target->method == POST && target->body != NULL) printf("%s\n", target->body);
}