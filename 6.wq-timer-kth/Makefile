obj-m += wtk.o

KDIR := /home/waveslike/vm/linux-6.17.9/
PWD := $(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
