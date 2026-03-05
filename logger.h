#ifndef LOGGER_H_
#define LOGGER_H_

#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "util.h"

typedef struct {
  int pipe_fd;
} mprex_logger; 

typedef struct { 
	ssize_t len;
  uint8_t flag;
  void *data;
} mprex_log;

typedef struct {
  ssize_t len;
  void *data;
} mprex_log_data;

typedef struct {
  unsigned long *ctxt_id;
  mprex_log_data *request;
  mprex_log_data *response;
} mprex_ctxt;

pthread_mutex_t logger_mtx;
unsigned long curr_ctxt = 1;

int mprex_logger_get_mutex() {
  return pthread_mutex_lock(&logger_mtx);
}

int mprex_logger_release_mutex() {
  return pthread_mutex_unlock(&logger_mtx);
}

mprex_ctxt *mprex_ctxt_init() {
  return (mprex_ctxt *)malloc(sizeof(mprex_ctxt));
}


mprex_ctxt *mprex_ctxt_search(mprex_ctxt *arr, unsigned long ctxt_id) {
  uint32_t count = ARRAY_COUNT(arr);
  for(int i = 0; i < count; i++) {
    if(array[i]->ctxt_id == ctxt_id) {
      return (void *)array[i];
    }
  }

  return NULL;
}

static void *mprex_logger_routine(void *data) {
  mprex_logger *logger = (mprex_logger*) data;

	pthread_mutexattr_t mtxattr;
	pthread_mutexattr_init(&mtxattr);
  pthread_mutex_init(&logger_mtx, &mtxattr);

  mprex_ctxt *contexts;
  ARRAY_INIT(contexts);

  struct pollfd pfd = {
    .fd = logger->pipe_fd,
    .events = POLLIN,
    .revents = 0,
  };
  int pvalue;
	ssize_t len = 0;
  unsigned long ctxt = 0;
  uint8_t flag = 255;
  void *mem;
  while((pvalue = poll(&pfd, 1, -1)) >= 0) {
    if(pvalue == 0) continue;
    if(pfd.revents & POLLIN) {
      void *buff = malloc(1024);
      ssize_t bytes = read(logger->pipe_fd, buff, 1024);
      ssize_t offset = 0;
      if(len == 0 && bytes >= sizeof(len)) {
        memcpy(&len, buff, sizeof(len));
        mem = malloc(len);
        offset += sizeof(len);
      } 
      if(ctxt == 0 && bytes - offset >= sizeof(ctxt)) {
        memcpy(&ctxt, buff+offset, sizeof(ctxt)); 
        offset += sizeof(ctxt);
      } 
      if(flag == 255 && bytes - offset >= sizeof(flag)) {
        memcpy(&flag, buff+offset, sizeof(flag));
        offset += sizeof(flag);
      }
      memcpy(mem, buff+offset, bytes-offset);
    }
  }

  ARRAY_FREE(contexts);

  pthread_exit(NULL);
  return NULL;
}

int mprex_logger_start() {
	int pipefd[2];
	pipe(pipefd);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_t tid;
	mprex_logger *data = malloc(sizeof(mprex_logger));
	data->pipe_fd = pipefd[0];
	int result = pthread_create(&tid, &attr, &mprex_logger_routine, (void*) data);
	pthread_detach(tid);

	pthread_attr_destroy(&attr);

	ssize_t len = 56;
	write(pipefd[1], &len, sizeof(ssize_t));
	void *value = malloc(2049);
}

unsigned long mprex_logger_create_ctxtid() {
  return curr_ctxt++;
}

int mprex_log_request(void *req) {}
int mprex_log_response(void *res) {}

#endif // LOGGER_H_
