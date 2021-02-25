all : server client

server: server.o
	gcc -o server server.o -lrt -pthread -Wall -Wextra -pedantic

server.o : server.c
	gcc -c server.c 

client: client.o
	gcc -o client client.o -lrt -pthread -Wall -Wextra -pedantic

client.o : client.c
	gcc -c client.c  

clean:
	rm -f server *.o
