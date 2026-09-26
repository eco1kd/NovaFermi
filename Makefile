# Makefile for building out-of-tree kernel modules under Arch Linux (kbuild)

obj-m += nv_fermi_drv.o
nv_fermi_drv-objs := core.o vbios.o i2c.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod nv_fermi_drv.ko

unload:
	sudo rmmod nv_fermi_drv

install:
	sudo cp nv_fermi_drv.ko /lib/modules/$(shell uname -r)/extra/
	sudo depmod -a

log:
	sudo dmesg -w | grep nv_fermi_drv

help:
	@echo "Targets:"
	@echo "  all     - build the kernel module"
	@echo "  clean   - remove build artifacts"
	@echo "  load    - insmod the built module"
	@echo "  unload  - rmmod the module"
	@echo "  install - copy module and run depmod"
	@echo "  log     - tail dmesg for driver output"

.PHONY: all clean load unload install log help
