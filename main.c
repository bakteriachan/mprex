#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>

#include "http.h"
#include "request.h"
#include "response.h"

#define LISTEN_BACKLOG 50

int proxy_fd, client_fd, server_fd;
char *server_ip, *server_port;
struct sockaddr_in proxy_addr, client_addr, server_addr;

void server_connect() {
  server_fd = socket(AF_INET, SOCK_STREAM, 0); 
  socklen_t len = sizeof(server_addr);
  if(connect(server_fd, (struct sockaddr *)&server_addr, len) < 0) {
    fprintf(stderr, "error: server connect: %s\n", strerror(errno));
  }
}

char *get_time_str() {
  time_t rawtime;
  struct tm *timeinfo;
  char *buffer = malloc(80);
  time(&rawtime); 
  timeinfo = localtime(&rawtime);
  strftime(buffer, 80, "%T", timeinfo);

  return buffer;
}

char *headers_build_str(struct LinkedList *headers) {
	char *buff = malloc(200);
  buff[0] = '\0';

	struct Header header;
	struct LinkedList curr = *headers;
	while(curr.data != NULL && curr.next != NULL) {
		header = *(struct Header *) curr.data;
		char *header_str = malloc(strlen(header.key) + strlen(header.value) + 5);
		sprintf(header_str, "%s: %s\r\n", header.key, header.value);
		buff = realloc(buff, strlen(buff) + strlen(header_str));
    strcat(buff, header_str);
		curr = *curr.next;
	}

	buff = realloc(buff, strlen(buff) + 2);
  strcat(buff, "\r\n");

	return buff;
}

void send_request(struct Request *request) {
  printf("connecting to server\n");
	char *request_str = malloc(4096 * 2);
	char *headers_str = headers_build_str(request->headers);
	sprintf(request_str, "%s %s %s\r\n", request->method, request->uri, request->proto);
  strcat(request_str, headers_str);
  strcat(request_str, request->body);

  server_connect();
  if(send(server_fd, request_str, strlen(request_str), 0) < 0) {
    printf("server send error: %s\n", strerror(errno));
  }

  int len = 0;
  char *server_response = NULL;
  char *buff = malloc(4096);

  ssize_t bytes;
  while((bytes = recv(server_fd, buff, 4096, 0)) > 0) {
    len += bytes;
    printf("bytes: %ld\n", bytes);
    server_response = realloc(server_response, len);
    strcat(server_response, buff);
  }

  struct Response *res = malloc(sizeof(struct Response));

  response_build(server_response, res);

  response_disect(res);

  close(server_fd);
}

void process_request(struct Request *request) {
	char command[200];
  char *time_str = get_time_str();
	printf("> [%s] [%s] %s $ ", time_str, request->method, request->uri);
  free(time_str);
	scanf("%s", command);
	if(strcmp(command, "disect") == 0) {
		disect_request(request, 0);
    process_request(request);
	}
	if(strcmp(command, "resend") == 0) {
		send_request(request);
	}
}

void serve() {
  proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
  proxy_addr.sin_family = AF_INET;
  proxy_addr.sin_port = htons(3000);
  proxy_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  int enable = 1;
  setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); 
  if(bind(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
    fprintf(stderr, "bind: error %s\n", strerror(errno));
    exit(1);
  }


  listen(proxy_fd, LISTEN_BACKLOG);
  printf("Server listening on PORT 3000\n");

  socklen_t peer_addr_len = sizeof(client_addr);
  while(1) {
    printf("Waiting for connections\n");
    client_fd  = accept(proxy_fd, (struct sockaddr *)&client_addr, &peer_addr_len);
    if(client_fd == -1) {
      printf("error: socket accept");
      return;
    }
    char *buff = malloc(4096); 
    size_t len = 4096;
    int bytes = recv(client_fd, buff, len, 0);
    buff[bytes] = '\0';

    struct Request *req = malloc(sizeof(struct Request));
    request_build(buff, req);

		process_request(req);

		close(client_fd);
    free(req);
  }
}

void signal_sigint(int s) {
	printf("signal: %d\n", s);
	close(proxy_fd);
	exit(1);
}

int main(int argc, char *argv[]) {
  if(argc != 3) {
    printf("Usage: %s ip port\n", argv[0]);
    return 1;
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[2]));
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	signal(SIGINT, signal_sigint);
  serve();
  return 0;
}
