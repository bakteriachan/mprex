#include <sys/socket.h>
#include <errno.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define LISTEN_BACKLOG 50

struct LinkedList {
  void *data;
  struct LinkedList *next;
};

void list_append(void *data, struct LinkedList *list) {
  if(list == NULL) {
    list = malloc(sizeof(struct LinkedList));
    list->next = NULL;
  }

  while(list->next != NULL) {
    list = list->next;
  }
  list->data = data;
  list->next = malloc(sizeof(struct LinkedList));
  list = list->next;
  list->next = NULL;
}

struct Header {
  char *key;
  char *value;
};

struct Request {
  char *method;
  char *uri;
  char *proto;
  struct LinkedList *headers;
  size_t headers_len;
  char *body;
};

struct Response {
  char *proto;
  uint32_t status;
  char *reason;
  struct Header *headers;
  size_t headers_len;
  char *body;
};

void header_build(char *line, struct Header *header) {
  size_t len = strlen(line);
  char buff[4096];
  size_t curr = 0;
  for(size_t i = 0; i < len; ++i) {
    if(line[i] == ':' && line[i+1] == ' ') {
      buff[curr] = '\0';
      header->key = malloc(strlen(buff) + 1);
      strcpy(header->key, buff);

      ++i;
      curr = 0;

      continue;
    } 
    buff[curr] = line[i];
    curr += 1;
  }

  buff[curr] = '\0';
  header->value = malloc(strlen(buff) + 1);
  strcpy(header->value, buff);
}

size_t headers_build(char *buff, struct LinkedList *headers) {
  int cnt = 0;
  size_t len = 0;
  size_t i = 0;

  while(cnt == 0) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      cnt++;
      i += 2;
      break;
    }
    ++i;
  }

  cnt = 0;
  char line[4096];
  int idx = 0;
  size_t buff_len = strlen(buff);
  while(cnt < 2 && i < buff_len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      cnt++;
      i += 2;
      if(cnt == 1) {
        line[idx+1] = '\0';
        idx = 0;
        len++;

        printf("line: %s\n", line);
        struct Header *header = malloc(sizeof(struct Header));
        header_build(line, header);
        list_append(header, headers);
      }
      continue;
    }
    if(cnt == 1) cnt = 0;
    line[idx] = buff[i]; 
    idx++;
    i++;
  }

  return len;
}

void request_build(char *buff, struct Request* req) {
  size_t len = strlen(buff);
  if(req == NULL) {
    req = malloc(sizeof(struct Request));
  }
  req->method = malloc(15);
  req->uri = malloc(4096);
  int i = 0, idx = 0;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      req->method[idx] = '\0';
      i += 2;
      break;
    }
    if(buff[i] == ' ') break;
    req->method[idx] = buff[i];
    ++i;
    ++idx;
  }

  idx = 0;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      req->uri[idx] = '\0';
      i+=2;
      break;
    }
    if(buff[i] == ' ') break;
    *(req->uri+idx) = buff[i];
    ++i;
    ++idx;
  }

  idx = 0;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      req->proto[idx] = '\0';
      break;
    }
    if(buff[i] == ' ') break;
    req->proto[idx] = buff[i];
    ++i;
    ++idx;
  }

  req->headers = malloc(sizeof(struct Header));
  req->headers_len = headers_build(buff, req->headers);
}

void serve() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr, caddr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(3000);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "bind: error %s\n", strerror(errno));
    exit(1);
  }


  listen(fd, LISTEN_BACKLOG);

  socklen_t peer_addr_len = sizeof(caddr);
  while(1) {
    int cfd = accept(fd, (struct sockaddr *)&caddr, &peer_addr_len);
    if(cfd == -1) {
      printf("error: socket accept");
      return;
    }
    printf("%d\n", cfd);
    char *buff = malloc(4096); 
    size_t len = 4096;
    int bytes = recv(cfd, buff, len, 0);
    buff[bytes] = '\0';

    struct Request *req = malloc(sizeof(struct Request));
    request_build(buff, req);

    if(bytes > 0) {
      printf("%s\n", buff);
    }
    strcpy(buff, "HTTP/1.1 200 HUHU\n");
    len = strlen(buff);
    send(cfd, buff, len, 0);
    close(cfd);
  }
}

int main() {
  serve();
  return 0;
}
