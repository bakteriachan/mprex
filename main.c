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
int nostdin = 0;

int server_connect() {
  server_fd = socket(AF_INET, SOCK_STREAM, 0); 
  socklen_t len = sizeof(server_addr);
  if(connect(server_fd, (struct sockaddr *)&server_addr, len) < 0) {
    fprintf(stderr, "error: server connect: %s\n", strerror(errno));
    return -1;
  }
}

char *get_time_str(time_t *ttime) {
  time_t rawtime;
  if(ttime != NULL) {
    rawtime = *ttime;
  } else {
    time(&rawtime); 
  }

  struct tm *timeinfo;
  char *buffer = malloc(80);
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
    buff = realloc(buff, strlen(buff) + strlen(header_str) + 1);
    strcat(buff, header_str);
    curr = *curr.next;

    free(header_str);
  }

  buff = realloc(buff, strlen(buff) + 5);
  strcat(buff, "\r\n");

  return buff;
}

void client_send_response(struct Response *res) {
  char *response_str = malloc(4096 * 2 + res->content_len);
  char *headers_str = headers_build_str(res->headers);
  sprintf(response_str, "%s %u %s\n", res->proto, res->status, res->reason);
  strcat(response_str, headers_str);
  size_t len = strlen(response_str) + res->content_len;
  if(res->body != NULL)
    memcpy(response_str + strlen(response_str), res->body, res->content_len);

  send(client_fd, response_str, len, 0); 

  free(headers_str);
  free(response_str);
}

void process_server_response(struct Response *res) {
  char *time_str = get_time_str(res->ttime);
  char command[200];
  printf("> [%s] [%u]", time_str, res->status);
  free(time_str);
  if(nostdin) {
    printf("\n");
    client_send_response(res);
    return;
  } else {
    printf(" $ ");
    scanf("%s", command);
  }

  if(strcmp(command, "disect") == 0) {
    response_disect(res);
    process_server_response(res);
  }
  if(strcmp(command, "continue") == 0 || strcmp(command, "c") == 0) {
    client_send_response(res);
    return;
  }

  process_server_response(res);
}

void send_request(struct Request *request) {
  char *request_str = malloc(4096 * 2);
  char *headers_str = headers_build_str(request->headers);
  sprintf(request_str, "%s %s %s\r\n", request->method, request->uri, request->proto);
  strcat(request_str, headers_str);
  free(headers_str);
  if(request->body != NULL)
    strcat(request_str, request->body);

  if(server_connect() < 0)
    return;
  if(send(server_fd, request_str, strlen(request_str), 0) < 0) {
    printf("server send error: %s\n", strerror(errno));
    return;
  }

  ssize_t len = 0;
  void *server_response = NULL;

  void *buff = malloc(4100);

  ssize_t bytes;
  while((bytes = recv(server_fd, buff, 4096, 0)) > 0) {
    server_response = realloc(server_response, len + bytes);
    memcpy(server_response+len, buff, bytes);
    len += bytes;
  }

  struct Response *res = malloc(sizeof(struct Response));
  res->headers = NULL;

  response_build(server_response, res);
  if(res->status == 431) {
    printf("%u\n", res->status);
  }


  process_server_response(res);

  response_free(res);

  close(server_fd);
  free(request_str);
  free(buff);
  free(server_response);
}

void process_request(struct Request *request) {
  char command[200];
  char *time_str = get_time_str(request->ttime);
  printf("< [%s] [%s] %s", time_str, request->method, request->uri);
  free(time_str);
  if(nostdin) {
    printf("\n");
    send_request(request);
    return;
  } else {
    printf(" $ ");
    scanf("%s", command);
  }
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
    client_fd  = accept(proxy_fd, (struct sockaddr *)&client_addr, &peer_addr_len);
    if(client_fd == -1) {
      printf("error: socket accept");
      return;
    }
    char *buff = malloc(4096); 
    size_t len = 4096;
    size_t bytes, req_len = 1;
    char *request_str = malloc(1);
    *request_str = '\0';
    do {
      bytes = recv(client_fd, buff, len, 0);
      if(bytes <= 0) break;
      req_len += bytes;
      request_str = realloc(request_str, req_len);
      strncat(request_str, buff, bytes);
    }while(bytes == len);

    struct Request *req = malloc(sizeof(struct Request));
    request_build(request_str, req_len, req);

    process_request(req);

    close(client_fd);
    request_free(req);
  }
}

void signal_sigint(int s) {
  printf("signal: %d\n", s);
  close(proxy_fd);
  exit(1);
}

int main(int argc, char *argv[]) {
  if(argc < 3 || argc > 4) {
    printf("Usage: %s ip port [-no-stdin]\n", argv[0]);
    return 1;
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[2]));
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);

  if(argc == 4) {
    if(strcmp(argv[3], "-no-stdin") == 0) {
      nostdin = 1;
    } else {
      printf("Usage: %s ip port [-no-stdin]\n", argv[0]);
      return 1;
    }
  }

  signal(SIGINT, signal_sigint);
  serve();
  return 0;
}
