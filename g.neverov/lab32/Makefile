all: server client1 client2

server: server.c
	gcc -o server server.c -lrt

client1: client1.c
	gcc -o client1 client1.c

client2: client2.c
	gcc -o client2 client2.c

runall: all
	@echo "Запускаем сервер..."
	./server &
	sleep 1
	@echo "Запускаем первого клиента..."
	./client1 &
	@echo "Запускаем второго клиента..."
	./client2 &
	wait
