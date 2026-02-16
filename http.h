#ifndef HTTP_H_
#define HTTP_H_

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "header.h"
#include "util.h"

struct Request {
  char *method;
  char *uri;
  char *proto;
  struct LinkedList *headers;
  size_t headers_len;
  char *body;
  time_t *ttime;
};

struct Response {
  char *proto;
  uint32_t status;
  char *reason;
  struct LinkedList *headers;
  size_t content_len;
  size_t headers_len;
  char *body;
  time_t *ttime;
};

size_t http_build_headers(char *buff, struct LinkedList **headers) {
  size_t len = 0;
  char *line = strchr(buff, '\n') + 1;
  while(*line != '\r') {
    char *key = substring(line, ':');
    line = strchr(line, ' ') + 1;
    char *value = substring(line, '\r');

    struct Header *header = header_build(key, value);
    ll_append(headers, header, sizeof(struct Header), header_free);

    free(key);
    free(value);
    free(header);

    len += 1;
    line = strchr(line, '\n') + 1;
  }

  return len;
}

struct Header *http_header_get(struct LinkedList *headers, const char *key) {
  struct Header *header = NULL;

  struct LinkedList *p = headers;

  struct Header *curr;
  while(p->data != NULL) {
    curr = (struct Header *) p->data;
    if(strncasecmp(curr->key, key, strlen(key)) == 0) {
      header = header_build(curr->key, curr->value);
      break;
    }
    p = p->next;
  }
  return header;
}

#endif // HTTP_H_
