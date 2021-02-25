all : program

program : program.o
	gcc -o program program.o -lrt -pthread

program.o : program.c
	gcc -c program.c 

clean:
	rm -f program *.o
