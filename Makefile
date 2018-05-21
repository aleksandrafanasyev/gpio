.PHONY : All target host clean libs

SHELL := /bin/sh
CC = gcc
C_FILES := gpio.c
CFLAGS = -Wall -Werror
LDFLAGS =
target: DIR := target
host: DIR := host

All: host

target: gpio

host: gpio

gpio: $(C_FILES)
	mkdir $(DIR)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(C_FILES) $(LDFLAGS) -o $(DIR)/gpio

clean:
	rm -f ./target/gpio
	rm -f ./host/gpio
	rm -f *.o
	rm -rf ./target
	rm -rf ./host





