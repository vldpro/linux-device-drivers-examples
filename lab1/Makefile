KERN_MOD = driver
obj-m = $(KERN_MOD).o
driver-objs := ./src/commands.o ./src/driver.o
PWD = $(shell pwd)/
MODULES_BUILD_PATH = /lib/modules/$(shell uname -r)/build

all:
	make -C "$(MODULES_BUILD_PATH)" M="$(PWD)" modules

clean:
	make -C "$(MODULES_BUILD_PATH)" M="$(PWD)" clean

insmod:
	sudo rmmod $(KERN_MOD); sudo insmod $(KERN_MOD).ko && sudo chown ${USER} /dev/chdev

test: insmod
	echo "lskdgj" >/tmp/MY_FILE && dmesg
