#include "server_main.h"


int isFileExists(char *filename)
{
    struct stat buf;
    int exists = stat(filename, &buf);

    return exists == 0;
}


int getFileSize(char *filename)
{
	FILE *fp = fopen(filename, "r");

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);

	fclose(fp);

	return size;
}


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

    char *buf = str_alloc(BUF_SIZE);
    int i;
    
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
                        int recvCount = 0;

                        int process; // 1이면 get, 2면 post
                        struct HttpRequestMessage *req;

                        FILE *fp;

                        char *filename;
                        int n = 0;

                    	int recvFileLength = 0;

                        while((recvSize = read(i, buf, BUF_SIZE)) > 0)
                        {
                            if(buf != NULL)
                            {
                            	if(recvCount == 0) // 헤더가 포함된 receive
                            	{
                            		req = parseRequestMessage(buf);
                            		if(req == NULL)
                            		{
                            			char *badRequest = makeString("HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
                            			write(i, badRequest, strlen(badRequest));
                            			process = -1;
                            			break;
                            		}

                            		if(req->method == GET)
                            		{
                            			process = GET;
                            			break;
                            		}
                            		else if(req->method == POST) // POST로 받은 상황
                            		{
                            			process = POST;
                            			filename = str_alloc(strlen(req->path)-1);
			                        	substr(filename, req->path + 1, req->path + strlen(req->path));
			                        	int payloadLength = recvSize - req->headerLength;

			                        	if(isFileExists(filename) == 1) // 존재하면
			                        	{
			                        		remove(filename);
			                        	}

		                        		fp = fopen(filename, "w");
		                        		
		                        		if(payloadLength > 0)
		                        		{
		                        			fwrite(req->body, 1, payloadLength, fp);
		                        			recvFileLength += payloadLength;
		                        			
                            				//printf("recvFileLength = %d, req->contentLength = %d with head\n", recvFileLength, req->contentLength);

                            				if(recvFileLength >= req->contentLength) // 다 받았으면 루프 탈출
				                    		{
				                    			fclose(fp);
				                    			break;
				                    		}
		                        		}

										free(filename);
                            		}
                            	}
                            	else // payload
                            	{
                            		fwrite(buf, 1, recvSize, fp);
                            		recvFileLength += recvSize;
                            		//printf("recvFileLength = %d, req->contentLength = %d\n", recvFileLength, req->contentLength);

                            		if(recvFileLength >= req->contentLength) // 다 받았으면 루프 탈출
                            		{
                            			fclose(fp);
                            			break;
                            		}
                            	}
				                
				                //write(1, buf, recvSize);
				                memset(buf, 0, strlen(buf) + 1);
				                recvCount++;
                            }
                        }

                        if(process == GET)
                        {
                        	filename = str_alloc(strlen(req->path)-1);
                        	substr(filename, req->path + 1, req->path + strlen(req->path));

                        	if(isFileExists(filename) != 1) // 존재 안하면
                        	{
                        		char *badRequest = makeString("HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n");
                            	write(i, badRequest, strlen(badRequest));
                        	}
                        	else // 존재하면
                        	{
                        		memset(buf, 0, BUF_SIZE);
                        		int contentLength = getFileSize(filename);

                        		char *header = str_alloc(100);
                        		sprintf(header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n", contentLength);

                        		fp = fopen(filename, "r");

                        		write(i, header, strlen(header));

                  				int count = 0;

                        		while((n = fread(buf, 1, BUF_SIZE, fp)) > 0) // 주의: strlen이 아닌 n값을 사용해야 한다.
							    {
							        write(i, buf, n);
							        memset(buf, 0, BUF_SIZE);

							        count += n;
							    }

							    free(header);
							    free(filename);
							    fclose(fp);
                        	}
                        }
                        else if(process == POST)
                        {
                        	char *header = makeString("HTTP/1.0 200 OK\r\nContent-Length: 0\r\n\r\n");
                        	write(i, header, strlen(header));

                        	free(header);
                        }

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
}


int main(int argc, char *argv[])
{
	short port = 0;
	parseArguments(argc, argv, &port);

	startServer(port);

	return 0;
}