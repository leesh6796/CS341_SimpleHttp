#include "server_main.h"

/*
    Parse arguments with getopt(), and return whether is -G or -P. Next, malloc and set URL to optarg value.
    Caution! you have to free *URL when program is closed.
*/
int parseArguments(int argc, char *argv[], short *port)
{
    int c = 0;

    while((c = getopt(argc, argv, "p:")) != -1)
    {
        if(c == '?') // 인식 오류나면
        {
            if(optopt == 'p') printf("client needs port number\n");
            
            return ERR;
        }
        else // 인식 오류 안나면
        {
            *port = (short)atoi(optarg); // port를 int로 바꿔서 넣어준다.
        }
    }

    return 0;
}


int startServer(short port)
{
    struct HttpServer *http = setupHttpServer(port);

    fd_set iter;
    int selector, num_fd;
    struct timeval timeout;

    memset(&http->addr_client, 0, sizeof(http->addr_client));

    FD_ZERO(&http->fd_sockets);
    FD_SET(http->sock_server, &http->fd_sockets);
    num_fd = http->sock_server; // 관찰 범위 설정

    int i;
    char *buf = str_alloc(FILE_SIZE);

    struct HttpRequestMessage *req = NULL;

    while(1)
    {
        iter = http->fd_sockets; // fd_sockets에 직접 하면 FD_SET을 다시 해줘야 한다.
        timeout.tv_sec = 15;
        timeout.tv_usec = 50000;

        selector = select(num_fd + 1, &iter, 0, 0, &timeout); // 첫 번째 인자를 max + 1 해서 넘겨야 한다.

        switch(selector)
        {
            case -1: return -4;
            case 0: break;
            default:
            // 실제 클라이언트의 요청을 수행하는 부분
            for(i=0; i<=num_fd; i++)
            {
                if(FD_ISSET(i, &iter)) // i번째 socket descriptor에 변화가 있으면
                {
                    if(i == http->sock_server) // i가 server socket이면 (새로운 접속 요청)
                    {
                        int len = sizeof(http->addr_client);
                        http->sock_client = accept(http->sock_server, (struct sockaddr*)&http->addr_client, &len);
                        if(http->sock_client < 0) return -5;

                        FD_SET(http->sock_client, &http->fd_sockets); // 원본에 새로운 descriptor 정보를 새겨준다.

                        if(num_fd < http->sock_client)
                        {
                            num_fd = http->sock_client;
                        }
                    }
                    else // i가 client socket이면 (이미 연결된 클라이언트에서의 요청)
                    {
                        int recvSize = 0;

                        while((recvSize = read(i, buf, FILE_SIZE)) >= 0)
                        {
                            /*if(recvSize == 0)
                            {
                                if(req != NULL)
                                    freeRequestMessage(req); // Request 끝났으므로 free 해준다.

                                FD_CLR(i, &http->fd_sockets);
                                close(i);
                            }
                            else
                            {*/
                            if(buf != NULL)
                            {
                                req = parseRequestMessage(buf);

                                char *version = makeString("HTTP/1.0");

                                if(req == NULL) // 400 Bad Request
                                {
                                    char *res = makeString("HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
                                    write(http->sock_client, res, strlen(res));
                                    free(res);
                                }
                                else
                                {   
                                    if(req->method == GET)
                                    {
                                        GetMethodProcess(http, req);
                                    }
                                    else if(req->method == POST)
                                    {
                                        PostMethodProcess(http, req);
                                    }
                                }

                                free(version);
                                break;
                            }
                            //}
                        }

                        if(req != NULL)
                            freeRequestMessage(req); // Request 끝났으므로 free 해준다.
                        
                        FD_CLR(i, &http->fd_sockets);
                        close(i);
                    }
                }
            }
        }
    }

    free(buf);

    close(http->sock_server);
    free(http);

    return 0;
}


struct HttpServer* setupHttpServer(short port)
{
    struct HttpServer *http = (struct HttpServer*)calloc(1, sizeof(struct HttpServer));

    http->sock_server = socket(PF_INET, SOCK_STREAM, 0);
    if(http->sock_server < 0) return -1;

    int one = 1;
    setsockopt(http->sock_server, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(http->sock_server, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)); // port를 재사용할 수 있게 만들어준다.

    memset(&http->addr_server, 0, sizeof(http->addr_server));

    http->addr_server.sin_family = AF_INET;
    http->addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    http->addr_server.sin_port = htons(port);

    if(bind(http->sock_server, (struct sockaddr*)&http->addr_server, sizeof(http->addr_server)) < 0) return -2;

    if(listen(http->sock_server, 5) < 0) return -3; // 최대 5명까지 접속 가능

    return http;
}


int GetMethodProcess(struct HttpServer *http, struct HttpRequestMessage *req)
{
    int end = strlen(req->path);
    char *path = str_alloc(end-1);
    substr(path, req->path + 1, req->path + end);
    
    int exists = isFileExists(path);

    struct HttpResponseMessage *res;
    
    if(exists == 1) // 파일 존재하면
    {
        sendFile(path, http, req);
    }
    else // 파일 없으면
    {
        char *res = makeString("HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n");
        write(http->sock_client, res, strlen(res));
        free(res);
    }

    free(path);

    return 0;
}


int PostMethodProcess(struct HttpServer *http, struct HttpRequestMessage *req)
{
    int end = strlen(req->path);
    char *path = str_alloc(end-1);
    substr(path, req->path + 1, req->path + end);

    if(isFileExists(path) == 1) // 존재하면
    {
        remove(path);
    }

    FILE *fp = fopen(path, "w");
    printf("%s\n", req->body);
    printf("%d\n", strlen(req->body));
    fwrite(req->body, 1, strlen(req->body), fp);
    fclose(fp);

    char *res = makeString("HTTP/1.0 200 OK\r\nContent-Length: 0\r\n\r\n");
    write(http->sock_client, res, strlen(res));
    free(res);

    free(path);

    return 0;
}


int isFileExists(char *filename)
{
    struct stat buf;
    int exists = stat(filename, &buf);

    return exists == 0;
}


int sendFile(char *filename, struct HttpServer *http, struct HttpRequestMessage *req)
{
    FILE *fp = fopen(filename, "r");
    struct HttpResponseMessage *res;
        
    fseek(fp, 0, SEEK_END); // 파일 포인터를 EOF까지 이동
    int contentLength = ftell(fp); // Content-Length를 구한다.
    fseek(fp, 0, SEEK_SET); // 파일 포인터를 다시 처음으로 이동

    const int BUF_SIZE = 1024;
    char *file = str_alloc(FILE_SIZE);
    char *fileBuf = str_alloc(BUF_SIZE);

    res = createResponseMessage(req->version, "200 OK", contentLength);
    char *buf = makeResponseHeaderString(res);

    strcat(file, buf);

    int n = 0;
    int len_file = 0;
    int len_header = strlen(file);

    while((n = fread(fileBuf, 1, BUF_SIZE, fp)) > 0)
    {
        //snprintf(file + cursor, n + 1, "%s", fileBuf);
        strncat(file, fileBuf, n);
        //cursor += n;
        len_file += n;
        memset(fileBuf, 0, sizeof(fileBuf));
    }

    int len_sum = len_file + len_header;
    write(http->sock_client, file, len_sum);

    freeResponseMessage(res);

    free(buf);
    free(file);
    free(fileBuf);

    fclose(fp);
    
    return 0;
}


int main(int argc, char *argv[])
{
    short port; // Server Port Number
    char *buf = NULL; // CAUTION! need to free

    parseArguments(argc, argv, &port); // port number를 파싱해온다.

    int run = 0;
    if((run = startServer(port)) < 0) printf("Error %d\n", run); // given port number로 HTTP Server를 시작한다.

    return 0;
}
