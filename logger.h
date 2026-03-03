#ifndef LOGGER_H_
#define LOGGER_H_

#include <poll.h>
#include <pthread.h>

typedef struct {
  int pipe_fd;
} mprex_logger; 

pthread_mutex_t logger_mtx;

int mprex_logger_get_mutex() {
  return pthread_mutex_lock(&logger_mtx);
}

int mprex_logger_release_mutex() {
  return pthread_mutex_unlock(&logger_mtx);
}

static void *mprex_logger_listen(void *data) {
  mprex_logger *logger = (mprex_logger*) data;

  //pthread_mutex_init();

  struct pollfd pfd = {
    .fd = logger->pipe_fd,
    .events = POLLIN,
    .revents = 0,
  };
  int pvalue;
  while((pvalue = poll(&pfd, 1, -1)) >= 0) {
    if(pvalue == 0) continue;
    if(pfd.revents & POLLIN) {
      
    }
  }

  pthread_exit(NULL);
  return NULL;
}

#endif // LOGGER_H_
