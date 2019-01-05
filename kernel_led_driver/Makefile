obj-m += ardled.o
#obj-m += leds-dac124s085.o

all:
	make -C  $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C  $(KERNEL_SRC) M=$(PWD) clean
