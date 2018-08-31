obj-m = lxdriver.o
lxdriver-objs = lx_acc_driver.o lis3dh_acc.o

KDIR := linux-headers-4.1.15-ti-rt-r43
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

all: lxaccelldriver lxaccellapp

lxaccelldriver:
	make -C $(KDIR) M=${shell pwd} modules ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	dtc -O dtb -o BB-LX-ACCEL-00A0.dtbo -b 0 -@ BB-LX-ACCEL-00A0.dts
	cp BB-LX-ACCEL-00A0.dtbo /lib/firmware/

lxaccellapp:
	$(CROSS_COMPILE)gcc lxapp.c -o lxapp

clean:
	make -C $(KDIR) M=${shell pwd} clean
