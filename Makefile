all: main mem

main: main.c
	gcc -g -pthread -o hw5_main main.c

mem: mem.c
	gcc -g -pthread -o mem mem.c
clean:
	rm -f hw5_main mem
	find . -type p -delete	

