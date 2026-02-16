mprex: main.c http.h request.h response.h
	cc main.c http.h request.h -o mprex

debug: main.c http.h request.h response.h util.h header.h
	cc -g main.c -o debug
