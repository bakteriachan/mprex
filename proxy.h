#ifndef PROXY_H_
#define PROXY_H_

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <poll.h>

#ifndef LISTEN_BACKLOG
#define LISTEN_BACKLOG 50
#endif

#ifndef _GNU_SOURCE
#define MPREX_POLLERROR (POLLERR | POLLHUP | POLLNVAL | POLLRDHUP)
#else
#define MPREX_POLLERROR (POLLERR | POLLHUP | POLLNVAL)
#endif

typedef struct {
  struct sockaddr_in *mprex_addr;
  struct sockaddr_in *server_addr;
} mprex_proxy;

typedef struct {
  int *fd;
  struct sockaddr_in *server_addr;
} mprex_client;


mprex_client *mprex_client_build(int fd, struct sockaddr_in *server) {
  mprex_client *client = malloc(sizeof(mprex_client));
  client->fd = malloc(sizeof(int));
  client->server_addr = malloc(sizeof(struct sockaddr_in));

  *client->fd = fd;
  memcpy(client->server_addr, server, sizeof(struct sockaddr_in));

  return client;
}

// poll for socket INPUT
int mprex_poll_socket(int fd) {
  struct pollfd pfd = {
    .fd = fd,
    .events = POLLIN,
    .revents = 0,
  };
  poll(&pfd, 1, 0);
  if(pfd.revents & MPREX_POLLERROR) return -1;
  if(pfd.revents & POLLIN) return 1;
  return 0;
}

static void *mprex_client_connected(void *data) {
  mprex_client *client = (mprex_client *)data;

  // TODO: send 500 HTTP error
  int server_fd = socket(AF_INET, SOCK_STREAM, 0); 
  if(connect(server_fd, (struct sockaddr *)client->server_addr, sizeof(struct sockaddr)) < 0) {
    fprintf(stderr, "connect error: %s\n", strerror(errno));
    close(*client->fd);
    return 0;
  }

  void *buff = malloc(1025);

  void *client_request = NULL, *server_response = NULL;
  size_t request_len = 0, response_len = 0;


  do {
    int mpoll = mprex_poll_socket(*client->fd);
    if(mpoll < 0) {
      fprintf(stderr, "socket connection error\n");
      break;
    }
    if(mpoll == 1) {
      ssize_t bytes = recv(*client->fd, buff, 1024, 0);
      if(bytes == 0) {
        break;
      } 

      client_request = realloc(client_request, request_len + bytes);
      memcpy(client_request + request_len, buff, bytes);
      request_len += bytes;

      bytes = send(server_fd, buff, bytes, 0);
    }
    

    mpoll = mprex_poll_socket(server_fd);
    if(mpoll < 0) {
      fprintf(stderr, "socket connection error\n");
      break;
    }

    if(mpoll == 1) {
      ssize_t bytes = recv(server_fd, buff, 1024, 0);
      server_response = realloc(server_response, response_len + bytes); 
      memcpy(server_response + response_len, buff, bytes);
      response_len += bytes;
      bytes = send(*client->fd, buff, bytes, 0);
    }
    
  } while(1);


  close(*client->fd);
  // TODO: log request and response

  return 0;
}


void mprex_listen(mprex_proxy *proxy) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0) {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    return;
  }

  int enable = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); 

  if(bind(fd, (struct sockaddr *)proxy->mprex_addr, sizeof(struct sockaddr_in)) < 0) {
    fprintf(stderr, "bind error: %s\n", strerror(errno));
    return;
  }

  listen(fd, LISTEN_BACKLOG);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  while(1) {
    struct sockaddr_in client_addr;
    socklen_t peer_addr_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr *) &client_addr, &peer_addr_len);
    if(client_fd < 0) {
      fprintf(stderr, "accept error: %s\n", strerror(errno));
      continue;
    }

    mprex_client *client = mprex_client_build(client_fd, proxy->server_addr); 

    pthread_t thid;
    pthread_create(&thid, &attr, &mprex_client_connected, (void *)client);
  }
  pthread_attr_destroy(&attr);
}

#endif // PROXY_H_
