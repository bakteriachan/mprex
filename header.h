#ifndef HEADER_H_
#define HEADER_H_

struct Header {
  char *key;
  char *value;
};

struct Header* header_build(char *key, char *value) {
  struct Header* header = malloc(sizeof(struct Header));
  header->key = malloc(strlen(key) + 1);
  header->value = malloc(strlen(value) + 1);
  strcpy(header->key, key);
  strcpy(header->value, value);

  return header;
}

#endif // HEADER_H_
