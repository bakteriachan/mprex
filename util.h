#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <string.h>

struct LinkedList {
  void *data;
  struct LinkedList *next;
};

struct LinkedList *ll_create(void *data, size_t len) {
  struct LinkedList *ll = malloc(sizeof(struct LinkedList));
  ll->next = NULL;
  ll->data = malloc(len);
  memcpy(ll->data, data, len);

  return ll;
}

void ll_append(struct LinkedList *list, void *data, size_t len) {
  if(list == NULL) {
    list = ll_create(data, len);
  } else {
    struct LinkedList *p = list;
    while(p->next != NULL) {
      p = p->next;
    }

    p->data = malloc(len);
    memcpy(p->data, data, len);
    p->next = NULL;
  }
}

char *substring(char *str, int delimiter) {
  char *pointer = strchr(str, delimiter);
  unsigned int len = pointer - str + 1;

  char *substr = malloc(len);
  memcpy(substr, str, len);
  substr[len-1] = '\0';

  return substr;
}

#endif // UTIL_H_
