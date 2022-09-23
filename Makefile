server: server.c
	gcc -Wall -pthread server.c -o server

clean:
	-rm -f server
