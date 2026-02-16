#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <string.h>

typedef void (*DataFreeFunc)(void *);

struct LinkedList {
  void *data;
  struct LinkedList *next;
  DataFreeFunc free_fn;
};

void ll_free(struct LinkedList *list) {
  struct LinkedList *next;
  struct LinkedList *curr = list;
  while(curr != NULL) {
    next = curr->next;
    if(curr->free_fn != NULL) {
      curr->free_fn(curr->data);
    } else {
      free(curr->data);
    }
    free(curr);

    curr = next;
  }
}

struct LinkedList *ll_create(void *data, size_t len, DataFreeFunc free_fn) {
  struct LinkedList *ll = malloc(sizeof(struct LinkedList));
  ll->data = malloc(len);
  memcpy(ll->data, data, len);

  ll->next = malloc(sizeof(struct LinkedList));
  ll->next->next = NULL;
  ll->next->data = NULL;
  if(free_fn != NULL)
    ll->free_fn = free_fn;
  else
    ll->free_fn = NULL;

  return ll;
}

void ll_append(struct LinkedList **list, void *data, size_t len, DataFreeFunc free_fn) {
  if(*list == NULL) {
    *list = ll_create(data, len, free_fn);
  } else {
    struct LinkedList *p = *list;
    int i = 1;
    while(p->next != NULL) {
      p = p->next;
      i++;
    }

    p->data = malloc(len);
    memcpy(p->data, data, len);
    p->next = malloc(sizeof(struct LinkedList));
    p->next->data = p->next->next = NULL;
    p->next->free_fn = NULL;

    p->free_fn = free_fn;

    p = *list;
  }
}

char *substring(char *str, int delimiter) {
  char *pointer = strchr(str, delimiter);
  if(!pointer) return NULL;

  unsigned int len = pointer - str;
  char *substr = malloc(len + 1);
  memcpy(substr, str, len);
  substr[len] = '\0';

  return substr;
}

#endif // UTIL_H_
