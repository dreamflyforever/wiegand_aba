obj-m :=readcard_auto.o

KDIR := /opt/workspace/linux-2.6.30.4
all:
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean 
