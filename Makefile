all: main mem

main: main.c
	gcc -pthread -o main main.c

mem: mem.c
	gcc -pthread -o mem mem.c 
