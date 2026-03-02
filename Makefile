HEADERS=proxy.h
SRC=main.c
CC=cc

mprex: $(SRC) $(OBJS) 
	$(CC) $(SRC) -pthread -o mprex

debug: $(SRC) $(OBJS) 
	$(CC) -g -pthread $(SRC) -o debug
