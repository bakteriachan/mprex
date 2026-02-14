#ifndef HTTP_H_
#define HTTP_H_

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

struct LinkedList {
  void *data;
  struct LinkedList *next;
};

void list_append(void *data, struct LinkedList *list) {
  if(list == NULL) {
    list = malloc(sizeof(struct LinkedList));
    list->next = NULL;
    list->data = NULL;
  }
  struct LinkedList *p = list;

  while(list->next != NULL) {
    list = list->next;
  }
  list->data = data;
  list->next = malloc(sizeof(struct LinkedList));
  list = list->next;
  list->next = NULL;
  list->data = NULL;

  list = p;
}

struct Header {
  char *key;
  char *value;
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
  size_t headers_len;
  char *body;
  time_t *ttime;
};

#endif // HTTP_H_
