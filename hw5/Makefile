all : floristApp

floristApp : floristApp.o
	gcc -o floristApp floristApp.o -pthread

floristApp.o : floristApp.c
	gcc -c floristApp.c -Wall -Wextra -pedantic -g  -I .

clean:
	rm -f floristApp *.o
