.PHONY : All target host clean libs

SHELL := /bin/sh
CC = gcc
C_FILES := aagpio.c
CFLAGS = -Wall -Werror
LDFLAGS =
target: DIR := target
host: DIR := host

All: host

target: aagpio

host: aagpio

aagpio: $(C_FILES)
	mkdir $(DIR)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(C_FILES) $(LDFLAGS) -o $(DIR)/aagpio

clean:
	rm -f ./target/aagpio
	rm -f ./host/aagpio
	rm -f *.o
	rm -rf ./target
	rm -rf ./host





