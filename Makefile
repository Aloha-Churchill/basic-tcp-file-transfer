COMPILER=gcc
FLAGS=-g -Wall

# make sure client and server are communicating on same port

all: client.c server.c
	$(COMPILER) $(FLAGS) -o client client.c
	$(COMPILER) $(FLAGS) -o server server.c
	./server 3490
	
run_client:
	./client localhost 3490 rabbit.jpg

clean:
	rm client
	rm server
