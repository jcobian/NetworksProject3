all: server

server: server.o
	g++ server.o -o server -lpthread
server.o: server.cpp ../chatinfo.h
	g++ -c server.cpp -Wall 
clean:
	rm -f core* *.o *~ server
