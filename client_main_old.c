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


/*
    string url을 파싱해 struct URL에 정보를 채워넣는다.
    http://ip:port/path
    url[7] : ip의 시작
*/
struct URL* parseURL(char *url_string)
{
    struct URL *url = (struct URL*)malloc(sizeof(struct URL));

    /*char *pure_url = str_alloc(strlen(url_string) - 7); // http://를 제외한 순수한 url, http://를 제외해야하므로 7칸을 뺀만큼 할당한다. +1은 \0
    strncpy(pure_url, url_string + 7, strlen(url_string) - 7);*/

    char *pure_url = copyString(url_string);

    char *port_start = index(pure_url, ':') + 1; // port 번호가 시작하는 문자열의 포인터
    char *path_start = index(pure_url, '/'); // 포트 번호가 끝나는 문자열 포인터 + 1
    int port_length = path_start - port_start; // 포트 길이를 계산

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


char* readBody() // POST에 넣을 body를 stdin으로 읽어옵니다.
{
    char c; // cursor
    unsigned int length = 0; // standard input length
    unsigned int bufSize = 32; // Input을 저장할 Buffer size

    char *buf = (char*)malloc(FILE_SIZE);
    //char *allocator = NULL;
    //unsigned int oldBufSize = 0; // realloc 하기 전의 buffer size

    memset(buf, 0, FILE_SIZE);

    while((c = getchar()) != EOF)
    {
        length++;
        /*if(bufSize < length)
        {
            oldBufSize = bufSize; // 원래 버퍼 사이즈 백업
            bufSize *= 2; // increaseFactor는 2^n으로 증가
            allocator = (char*)calloc(bufSize, sizeof(char));
            memcpy(allocator, buf, length - 1);
            free(buf);
            buf = allocator;

            if(buf == 0) // realloc 실패하면
            {
                printf("realloc 실패?\n");
            }
        }*/

        buf[length - 1] = c;
    }

    printf("%s\n", buf);
    printf("%d\n", length);

    return buf;
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
    if(conn < 0) return -2;

    char *msgString = makeRequestMessageString(message);
    write(sock, msgString, strlen(msgString));

    char *buf = str_alloc(FILE_SIZE);;

    int n;
    int sum = 0;
    while(1)
    {
        n = read(sock, buf, FILE_SIZE);
        sum += n;
        if(n > 0)
        {
            buf[n] = '\0';
            printf("%s", buf);
            //memset(buf, 0, n);
        }
        else
        {
            break;
        }
    }

    close(sock);
    free(msgString);
    free(buf);
}


int main(int argc, char *argv[])
{
    char *url_string; // CAUTION! need to free
    char *buf = NULL; // CAUTION! need to free

    int method = parseArguments(argc, argv, &url_string); // GET or POST
    struct URL *url = parseURL(url_string); // struct URL에 파싱한 url을 넣어줍니다.

    // POST 전송일 때는 stdin에서 input을 받아옵니다.
    if(method == POST)
    {
        buf = readBody();
    }

    char *version = makeString("1.0");
    struct HttpRequestMessage *message = createRequestMessage(method, url, version, buf);

    int req = 0;
    if((req = httpRequest(message)) < 0)
        printf("Error %d\n", req);

    // free
    freeRequestMessage(message);
    freeURL(url);

    free(url_string);
    free(version);
    free(buf); // buf는 freeRequestMessage에서 해제해주지 않는다.

    return 0;
}