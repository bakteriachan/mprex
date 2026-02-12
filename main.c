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
#include <signal.h>

#define LISTEN_BACKLOG 50

int fd, client_fd;

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

	printf("body: %s\n", buff+i);
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
	sprintf(buff, "");

	struct Header header;
	struct LinkedList curr = *headers;
	while(curr.data != NULL && curr.next != NULL) {
		header = *(struct Header *) curr.data;
		char *header_str = malloc(strlen(header.key) + strlen(header.value) + 5);
		sprintf(header_str, "%s: %s\r\n", header.key, header.value);
		buff = realloc(buff, strlen(buff) + strlen(header_str));
		sprintf(buff, "%s%s", buff, header_str);
		curr = *curr.next;
	}

	buff = realloc(buff, strlen(buff) + 2);
	sprintf(buff, "%s\r\n", buff);

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
	printf("%s\n", headers_str);
	sprintf(buff, "%s %s %s\r\n", request->method, request->uri, request->proto);
}

void process_request(struct Request *request) {
	char command[200];
	printf("Process Request\nChoose:\n(1) -> Disect\n(2) -> Resend\n(3) -> Modify\n> ");
	scanf("%s", &command);
	char *token = strtok(command, " ");
	if(strcmp(token, "disect") == 0) {
		token = strtok(command, " ");
		if(token == NULL) 
			disect_request(request, 0);
		if(strcmp(token, "main") == 0) 
			disect_request_main(request);
		if(strcmp(token, "headers") == 0)
			disect_request_headers(request);
		if(strcmp(token, "body") == 0)
			disect_request_body(request);
	}
	if(strcmp(token, "resend") == 0) {
		send_request(request);
	}
}

void serve() {
  fd = socket(AF_INET, SOCK_STREAM, 0);
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
    client_fd  = accept(fd, (struct sockaddr *)&caddr, &peer_addr_len);
    if(client_fd == -1) {
      printf("error: socket accept");
      return;
    }
    printf("%d\n", client_fd);
    char *buff = malloc(4096); 
    size_t len = 4096;
    int bytes = recv(client_fd, buff, len, 0);
    buff[bytes] = '\0';

    struct Request *req = malloc(sizeof(struct Request));
    request_build(buff, req);

    if(bytes > 0) {
      printf("%s\n", buff);
    }

		process_request(req);

		close(client_fd);
  }
}

void signal_sigint(int s) {
	printf("signal: %d\n", s);
	close(fd);
	exit(1);
}

int main() {
	signal(SIGINT, signal_sigint);
  serve();
  return 0;
}
