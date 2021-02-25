all : programA programB programC

programA : programA.o
	gcc -o programA programA.o

programA.o : programA.c
	gcc -c programA.c

programB : programB.o
	gcc -o programB programB.o -lm
	
programB.o : programB.c
	gcc -c programB.c

programC : programC.o
	gcc -o programC programC.o -lm

programC.o : programC.c
	gcc -c programC.c 

clean:
	rm -f programA programB programC *.o