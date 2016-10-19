CFLAGS=-Wall -Werror -Wshadow -Wextra
SRC=$(wildcard *.c) 
OBJ=$(SRC:.c=.o)

all:
	make clear;
	make receiver
	make sender

receiver:
	gcc $(CFLAGS) -o receiver src/receiver.c src/packet_implem.c src/real_address.c src/create_socket.c src/wait_for_client.c src/read_write_receiver.c -lz

sender:
	gcc $(CFLAGS) -o sender src/sender.c src/packet_implem.c src/real_address.c src/create_socket.c src/read_write_sender.c -lz

git:
	git add src/sender.c src/receiver.c src/real_address.c src/create_socket.c src/read_write_sender.c src/real_address.h src/create_socket.h src/read_write_sender.h src/packet_implem.c src/packet_interface.h  src/read_write_receiver.c src/read_write_receiver.h tests/test.c Makefile

test :
	rm test
	gcc $(CFLAGS) -o test tests/test.c src/packet_implem.c src/real_address.c src/create_socket.c src/wait_for_client.c src/read_write_receiver.c -lz -lcunit
	./test

clear : 
	rm -f sender
	rm -f receiver
