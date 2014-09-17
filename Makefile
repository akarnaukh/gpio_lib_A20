.PHONY : all clean
all: key

key:key.o gpio_lib.o
	@gcc key.o gpio_lib.o -o key

key.o:key.c
	@gcc -c key.c

gpio_lib.o:gpio_lib.c
	@gcc -c gpio_lib.c

clean:
	@rm -rf key *.o

