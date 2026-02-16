#ifndef RESPONSE_H_
#define RESPONSE_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "http.h"

void response_build(char *buff, struct Response *res) {
  if(res == NULL) {
    res = malloc(sizeof(struct Response));
    memset(res, 0, sizeof(res));
  }

  res->ttime = malloc(sizeof(time_t));
  time(res->ttime);

  res->proto = malloc(16);
  int i = 0;
  for(;buff[i] != ' '; ++i) {
    res->proto[i] = buff[i];
  }
  res->proto[i] = '\0';

  char *status = malloc(8);
  int idx = 0;
  for(i = i + 1;buff[i] != ' '; ++i, idx++) {
    status[idx] = buff[i];
  }
  status[idx] = '\0';
  res->status = atoi(status);
  free(status);

  res->reason = malloc(32);
  idx = 0;
  for(i = i + 1; buff[i] != '\r'; ++i, ++idx) {
    res->reason[idx] = buff[i];
  }
  res->reason[idx] = '\0';

  res->headers_len = http_build_headers(buff, &res->headers);

  struct Header *hcontent = http_header_get(res->headers, "content-length");
  if(hcontent != NULL) {
    size_t content_len = atoi(hcontent->value);
    void *body = malloc(content_len);
    memcpy(body, strstr(buff, "\r\n\r\n") + 4, content_len);
    res->content_len = content_len;
    res->body = malloc(content_len);
    memcpy(res->body, body, content_len);
    free(body);
  } else {
    res->content_len = 0;
    res->body = NULL;
  }

  header_free(hcontent);
}

void response_disect(struct Response *res) {
  printf("%s %u %s\n", res->proto, res->status, res->reason);
  struct LinkedList *curr = res->headers;
  while(curr != NULL) {
    if(curr->data == NULL) break;
    printf("%s: %s\n", ((struct Header *)curr->data)->key, ((struct Header *)curr->data)->value);
    curr = curr->next;
  }
  printf("\n");

  if(res->body != NULL)
    printf("%s\n", res->body);
}

void response_free(struct Response *res) {
  free(res->proto);
  free(res->reason);
  ll_free(res->headers);
  free(res->body);
  free(res->ttime);

  free(res);
}

#endif // RESPONSE_H_
