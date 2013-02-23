obj-m = 82540EMdrv.o
KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules
	gcc -o test82540EMdrv test82540EMdrv.c
clean:
	rm test82540EMdrv
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean

