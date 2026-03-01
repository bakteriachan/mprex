#ifndef PROXY_H_
#define PROXY_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#ifndef LISTEN_BACKLOG
#define LISTEN_BACKLOG 50
#endif

#define MPREX_POLLERROR (POLLERR & POLLHUP & POLLNVAL)

typedef struct {
  struct sockaddr_in mprex_addr;
  struct sockaddr_in server_addr;
} mprex_proxy;

typedef struct {
  int fd;
  struct sockaddr_in server_addr;
} mprex_client;


mprex_client mprex_client_build(int fd, struct sockaddr_in server) {
  mprex_client client = {
    .fd = fd,
    .server_addr = server,
  };
}

static void *mprex_client_connected(void *client) {
  mprex_client client = *(mprex_client *)client;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0); 
  if(connect(server_fd, (struct sockaddr *)&client.server_addr, sizeof(client.server_addr)) < 0) {
    fprintf(stderr, "connect error: %s\n", strerror(errno));
  }

  struct pollfd pfd;

  void buff[4096];

  void *client_request = NULL, *server_response = NULL;
  size_t request_len = 0, response_len = 0;


  do {
    pfd = {
      .fd = client.fd,
      events = POLLIN,
      revents = 0,
    };
    int rvalue = poll(&pfd, 1, -1);
    if(rvalue < 0) {
      fprintf(stderr, "poll error: %s\n", strerror(rvalue));
      break;
    }
    if(pfd.revents & MPREX_POLLERROR) {
      fprintf(stderr, "socket connection error\n");
      break;
    }
    ssize_t bytes = recv(fd, &buff, 1024, 0);
    if(bytes == 0) continue;

    client_request = realloc(client_request, request_len + bytes);
    request_len += bytes;
    memcpy(client_request, buff, bytes);

    send(server_fd, &buff, bytes, 0);

    pfd = {
      .fd = server_fd,
      .events = POLLIN,
      .revents = 0,
    };

    rvalue = poll(&pfd, 1, 0);
    if(rvalue < 0) {
      fprintf(stderr, "poll error: %s\n", strerror(rvalue));
      break;
    }
    if(pfd.revents & MPREX_POLLERROR) {
      fprintf(stderr, "socket connection error\n");
      break;
    }

    if(pfd.revents & POLLIN) {
      ssize_t bytes = recv(server);
      server_response = realloc(server_response, response_len + ); // ????
    }


    

  } while(1);

  close(fd);
}


void mprex_listen(mprex_proxy proxy) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0) {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    return;
  }

  int enable = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); 

  if(bind(fd, (struct sockaddr *)&proxy.mprex_addr, sizeof(proxy.mprex_addr)) < 0) {
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

    mprex_client client = mprex_client_build(client_fd, proxy.server_addr);

    pthread_t thid;
    pthread_create(&thid, &attr, &mprex_client_connected, (void *)&client);
  }
  pthread_attr_destroy(&attr);
}

#endif // PROXY_H_
