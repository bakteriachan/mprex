#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

typedef struct {
  uint32_t count;
  uint32_t capacity;
} ArrayHeader;

#define ARRAY_INIT(array) { \
  uint32_t capacity = 32; \
  ArrayHeader *header = malloc(sizeof(ArrayHeader) + sizeof(*array) * capacity); \
  header->count = 0; \
  header->capacity = capacity; \
  array = (void *) header + 1; \
} 

#define ARRAY_ADD(array, element) { \
  ArrayHeader *header = ((ArrayHeader *)array) - 1; \
  if(header->count >= array->capacity) {\
    header->capacity *= 2; \
    header = realloc(header, sizeof(ArrayHeader) + sizeof(*array) * header->capacity); \
    array = (void *)header + 1; \
  } \
  array[header->count] = element; \
  header->count++; \
} 

#define ARRAY_COUNT(array) ((ArrayHeader*)(array) - 1)->count

#define ARRAY_FREE(array) free((ArrayHeader *)array - 1)

#endif // UTIL_H_
