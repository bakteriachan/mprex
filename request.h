#ifndef REQUEST_H_
#define REQUEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"

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


void request_build(char *buff, struct Request* req) {
  size_t len = strlen(buff);
  if(req == NULL) {
    req = malloc(sizeof(struct Request));
  }
  req->ttime = malloc(sizeof(time_t));
  time(req->ttime);
  req->method = malloc(15);
  req->uri = malloc(4096);
	req->proto = malloc(15);
  int i = 0, idx = 0;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      i += 2;
      break;
    }
    if(buff[i] == ' ') break;
    req->method[idx] = buff[i];
    ++i;
    ++idx;
  }
  req->method[idx] = '\0';

	i += 1;
  idx = 0;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      i+=2;
      break;
    }
    if(buff[i] == ' ') break;
    req->uri[idx] = buff[i];
    ++i;
    ++idx;
  }
  req->uri[idx] = '\0';

  idx = 0;
	i+=1;
  while(i < len) {
    if(buff[i] == 13 && buff[i+1] == 10) {
      break;
    }
    req->proto[idx] = buff[i];
    ++i;
    ++idx;
  }
  req->proto[idx] = '\0';

	
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
  if(request->body != NULL)
	  printf("%s\n", request->body);
}

void disect_request(struct Request *request, int fl) {
		disect_request_main(request);
		disect_request_headers(request);
		printf("\n");
		disect_request_body(request);
}


#endif // REQUEST_H_

