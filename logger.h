#ifndef LOGGER_H_
#define LOGGER_H_

#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int pipe_fd;
} mprex_logger; 

typedef struct { 
	ssize_t len;
	void *data;
} mprex_log;

pthread_mutex_t logger_mtx;


int mprex_logger_get_mutex() {
  return pthread_mutex_lock(&logger_mtx);
}

int mprex_logger_release_mutex() {
  return pthread_mutex_unlock(&logger_mtx);
}

static void *mprex_logger_routine(void *data) {
  mprex_logger *logger = (mprex_logger*) data;

	pthread_mutexattr_t mtxattr;
	pthread_mutexattr_init(&mtxattr);
  pthread_mutex_init(&logger_mtx, &mtxattr);

  struct pollfd pfd = {
    .fd = logger->pipe_fd,
    .events = POLLIN,
    .revents = 0,
  };
  int pvalue;
  while((pvalue = poll(&pfd, 1, -1)) >= 0) {
    if(pvalue == 0) continue;
    if(pfd.revents & POLLIN) {
			ssize_t len;
    	read(logger->pipe_fd, &len, sizeof(len)); 
			printf("len: %lu\n", len);
    }
  }

  pthread_exit(NULL);
  return NULL;
}

int mprex_logger_start() {
	printf("uwu\n");
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

int mprex_logger_create_ctxtid() {
	return 0;
}

//int mprex_log(int ) {}

#endif // LOGGER_H_
