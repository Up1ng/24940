all: a b c

a: server.c
	gcc -o a server.c

b: client1.c
	gcc -o b client1.c

c: client2.c
	gcc -o c client2.c

run_server: a
	./a

run_client: b
	./b

run_client2: c
	./c

runall: all
	@echo "Запускаем сервер..."
	./a &
	sleep 1
	@echo "Запускаем первого клиента..."
	./b &
	@echo "Запускаем второго клиента..."
	./c &
	wait
