all: server

server: server.o
	g++ -pthread server.o -o server
server.o: server.cpp ../chatinfo.h
	g++ -pthread -c server.cpp -Wall
clean:
	rm -f core* *.o *~ server
