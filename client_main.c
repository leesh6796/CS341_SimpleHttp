#include "client_main.h"


/*
    Parse arguments with getopt(), and return whether is -G or -P. Next, malloc and set URL to optarg value.
    Caution! you have to free *URL when program is closed.
*/
int parseArguments(int argc, char *argv[], char **URL)
{
    int c = 0;

    while((c = getopt(argc, argv, "g:p:")) != -1)
    {
        if(c == '?') // 인식 오류나면
        {
            if(optopt == 'g' || optopt == 'p')
                    printf("client needs URL\n");
            
            return ERR;
        }
        else // 인식 오류 안나면
        {
            //*URL = (char*)malloc(strlen(optarg) + 1); // optarg로 들어온 url을 넣어준다.
            *URL = copyString(optarg);
            //memcpy(*URL, optarg, strlen(optarg) + 1);

            if(c == 'g') return GET;
            else if(c == 'p') return POST;
        }
    }

    return ERR;
}


struct URL* parseURL(char *url_string)
{
    struct URL *url = (struct URL*)malloc(sizeof(struct URL));

    /*char *pure_url = str_alloc(strlen(url_string) - 7); // http://를 제외한 순수한 url, http://를 제외해야하므로 7칸을 뺀만큼 할당한다. +1은 \0
    strncpy(pure_url, url_string + 7, strlen(url_string) - 7);*/

    char *pure_url = copyString(url_string);

    char *port_start = index(pure_url, ':') + 1; // port 번호가 시작하는 문자열의 포인터
    char *path_start = index(pure_url, '/'); // 포트 번호가 끝나는 문자열 포인터 + 1
    int port_length = path_start - port_start; // 포트 길이를 계산

    if(path_start == NULL)
    {
        path_start = pure_url + strlen(pure_url);
    }

    // 포트 파싱
    char *port = str_alloc(port_length);
    substr(port, port_start, path_start);

    // path 파싱
    char *path = str_alloc(strlen(path_start));
    substr(path, path_start, pure_url + strlen(pure_url));

    // ip 파싱
    char *ip = str_alloc(port_start - 1 - pure_url);
    substr(ip, pure_url, port_start - 1);

    url->ip = ip;
    url->port = port;
    url->path = path;

    free(pure_url);

    return url;
}


char* readBody(int *length)
{
    int n = 0;
    int len = 0;

    char *buf = str_alloc(BUF_SIZE);
    char *content = str_alloc(BUF_SIZE);
    content[0] = '\0';

    while((n = fread(buf, 1, BUF_SIZE, stdin)) > 0) // 주의: strlen이 아닌 n값을 사용해야 한다.
    {
        len += n;
        content = (char*)realloc(content, len);

        if(content == NULL)
        {
            printf("realloc error\n");
            exit(-1);
        }

        memcpy(content + len - n, buf, n);

        memset(buf, 0, BUF_SIZE);
    }

    *length = len;

    free(buf);

    return content;
}


int httpRequest(struct HttpRequestMessage *message)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0) return -1;

    struct sockaddr_in server_addr; // sockaddr는 IP, port가 한 변수 안에 들어있어 사용하기 불편하므로 sockaddr_in 사용
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(message->ip);
    server_addr.sin_port = htons(atoi(message->port));

    int conn = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(conn < 0)
    {
        printf("connect issue\n");
        return -2;
    }

    char *msgString = makeRequestHeaderString(message);
    write(sock, msgString, strlen(msgString));

    int sendFileLength = 0;
    int remain = message->contentLength;

    int j;

    if(message->method == POST)
    {
        int cursor = 0;
        while(cursor < message->contentLength)
        {
            int writeLength = BUF_SIZE < remain ? BUF_SIZE : remain;
            write(sock, message->body + cursor, writeLength);
            cursor += BUF_SIZE;

            remain -= writeLength;
            sendFileLength += writeLength;
        }
    }

    free(msgString);


    int recvSize = 0;
    char *buf = str_alloc(BUF_SIZE);

    int count = 0;

    while((recvSize = read(sock, buf, BUF_SIZE)) > 0)
    {
        if(buf != NULL)
        {
            write(1, buf, recvSize);
            memset(buf, 0, BUF_SIZE);

            count++;
        }
    }

    free(buf);

    close(sock);
}


int main(int argc, char *argv[])
{
    char *url_string;

    int method = parseArguments(argc, argv, &url_string); // GET or POST
    struct URL *url = parseURL(url_string); // struct URL에 파싱한 url을 넣어줍니다.

    char *body = NULL;
    int contentLength = 0;

    if(method == POST)
    {
        body = readBody(&contentLength);
    }

    char *version = makeString("1.0");
    struct HttpRequestMessage *message = createRequestMessage(method, url, version, body, contentLength);

    httpRequest(message);

    freeRequestMessage(message);
    freeURL(url);

    free(url_string);
    free(version);
    free(body); // buf는 freeRequestMessage에서 해제해주지 않는다.

    return 0;
}