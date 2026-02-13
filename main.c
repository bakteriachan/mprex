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
  header->value = malloc(strlen(buff));
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
        line[idx] = '\0';
        idx = 0;
        len++;

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

void body_build(char *buff, char *body) {
	size_t len = strlen(buff);
	size_t i = 0;
	int cnt = 0;
	while(cnt < 2 && i < len) {
		if(buff[i] == 13 && buff[i+1] == 10) {
			cnt += 1;
			i += 2;
		} else {
			cnt = 0;
			++i;
		}
	}
	if(body == NULL) {
		body = malloc(len - i + 1);
	}

	strcpy(body, buff+i);
}

int headers_get_content_length(struct LinkedList *list) {
	while(list != NULL && list->data != NULL) {
		struct Header header = *(struct Header *) list->data;
		if(strcasecmp(header.key, "content-length") == 0) {
			return atoi(header.value);
		}
		list = list->next;
	}
	return -1;
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

void request_build(char *buff, struct Request* req) {
  size_t len = strlen(buff);
  if(req == NULL) {
    req = malloc(sizeof(struct Request));
  }
  req->method = malloc(15);
  req->uri = malloc(4096);
	req->proto = malloc(15);
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

	i += 1;
  idx = 0;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      req->uri[idx] = '\0';
      i+=2;
      break;
    }
    if(buff[i] == ' ') break;
    req->uri[idx] = buff[i];
    ++i;
    ++idx;
  }

  idx = 0;
	i+=1;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      req->proto[idx] = '\0';
      break;
    }
    req->proto[idx] = buff[i];
    ++i;
    ++idx;
  }

	
  req->headers = malloc(sizeof(struct Header));
  req->headers->data = req->headers->next = NULL;
  req->headers_len = headers_build(buff, req->headers);
	int content_length = headers_get_content_length(req->headers);
	if(content_length > 0) {
		req->body = malloc(content_length);
	} else {
		req->body = NULL;
	}

	body_build(buff, req->body);
}

void disect_request_main(struct Request *request) {
		printf("%s %s %s\n", request->method, request->uri, request->proto);
}

void disect_request_headers(struct Request *request) {
	struct Header header;
	struct LinkedList curr = *request->headers;
	for(size_t i = 0; i < request->headers_len; i++) {
		header = *(struct Header *) curr.data;
		printf("%s: %s\n", header.key, header.value);
		curr = *curr.next;
	}
}

void disect_request_body(struct Request *request) {
	printf("%s\n", request->body);
}

void disect_request(struct Request *request, int fl) {
		disect_request_main(request);
		disect_request_headers(request);
		printf("\n");
		disect_request_body(request);
}

void send_request(struct Request *request) {
	char *buff = malloc(4096 * 2);
	char *headers_str = headers_build_str(request->headers);
	sprintf(buff, "%s %s %s\r\n", request->method, request->uri, request->proto);
  strcat(buff, headers_str);
  strcat(buff, request->body);

  server_connect();
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
