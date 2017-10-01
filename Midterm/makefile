all: 
	gcc -c  timeServer.c -lrt 
	gcc -o timeServer timeServer.o -lrt
	rm timeServer.o
	
	gcc -c   seeWhat.c -lm
	gcc -o seeWhat seeWhat.o -lm
	rm seeWhat.o

	gcc -c   showResults.c 
	gcc -o showResults showResults.o 
	rm showResults.o
clean:
	$(RM) timeServer
	$(RM) seeWhat
	$(RM) showResults

