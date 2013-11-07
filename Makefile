#Makefile for BeagleBoard

# Compiler options
CC = /usr/local/angstrom/arm/bin/arm-angstrom-linux-gnueabi-gcc
#FLAGS = -Wall -I -L -g -lpthread

#Remote deploy ( plz make sure SSH_IP is IP address of beagleboard )
SSH_IP = 172.16.10.114
SSH_USER = root
SSH_DIR = /home/root/Lab1

DRIVER_NAME = gmem-driver
APP_NAME = gmem-tester

obj-m := $(DRIVER_NAME).o
PWD := $(shell pwd)
KDIR := /home/rohit/cse598-ESP/kernel/kernel/

all:
	make -C $(KDIR) SUBDIRS=$(PWD) modules
	
app: $(APP_NAME) 
	$(CC) $(FLAGS) -o $(APP_NAME) $(APP_NAME).c

clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -f $(APP_NAME)

load-driver: 
	@echo "Copying ko to BeagleBoard..."
	scp $(DRIVER_NAME).ko $(SSH_USER)@$(SSH_IP):$(SSH_DIR)
	@echo "Done!"
	
load-app:
	@echo "Copying app to BeagleBoard..."
	scp $(APP_NAME) $(SSH_USER)@$(SSH_IP):$(SSH_DIR)
	@echo "Done!"


