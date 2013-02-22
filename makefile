all:server client

server:chatserverTCP.o
	gcc chatserverTCP.o -lpthread -o server

chatserverTCP.o:chatserverTCP.c
	gcc -c chatserverTCP.c

client:chatclientTCP.o
	gcc chatclientTCP.o -lpthread -o client

chatclientTCP.o:chatclientTCP.c
	gcc -c chatclientTCP.c    

clean:
	rm -rf *o server client
