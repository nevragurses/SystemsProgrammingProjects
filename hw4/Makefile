all : program

program : program.o
	gcc -o program program.o -pthread

program.o : program.c
	gcc -c program.c -Wall -Wextra -pedantic -g  -I .

clean:
	rm -f program *.o
