obj-m += ard101ph.o

all:
	make -C  $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C  $(KERNEL_SRC) M=$(PWD) clean