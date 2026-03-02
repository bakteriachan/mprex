#define _GNU_SOURCE

#include <signal.h>

#include "proxy.h"

#define LISTEN_BACKLOG 50

struct sockaddr_in proxy_addr, server_addr;
int PORT = 3000;
char host[16];

void signal_sigint(int s) {
  printf("signal: %d\n", s);
  exit(1);
}

void usage(char *name) {
	printf("Usage: %s ip port [-h <host>] [-p <port>]\n	-p <port>\n		port where the proxy will be listening\n	-h <host>\n		host where the proxy will be listening\nExample: %s 8.8.8.8 80  -h 0.0.0.0 -p 3000\n", name, name);
}

int main(int argc, char *argv[]) {
  if(argc < 3) {
		usage(argv[0]);
    return 1;
  }
	strcpy(host, "0.0.0.0");
	for(int i = 3; i < argc; i++) {
		if(strcmp(argv[i], "-h") == 0) {
			strcpy(host, argv[i+1]);
			++i;
		} else if(strcmp(argv[i], "-p") == 0) {
			PORT = atoi(argv[i+1]);
			++i;
		} else {
			usage(argv[0]);
			return 1;
		}
	}


  signal(SIGINT, signal_sigint);

  proxy_addr.sin_family = AF_INET;
  proxy_addr.sin_port = htons(PORT);
  proxy_addr.sin_addr.s_addr = inet_addr(host);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[2]));
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);


  mprex_proxy *proxy = malloc(sizeof(mprex_proxy));
  proxy->mprex_addr = malloc(sizeof(struct sockaddr_in));
  proxy->server_addr = malloc(sizeof(struct sockaddr_in));
  memcpy(proxy->mprex_addr, &proxy_addr, sizeof(struct sockaddr_in));
  memcpy(proxy->server_addr, &server_addr, sizeof(struct sockaddr_in));

  mprex_listen(proxy);
  return 0;
}
