.PHONY: all clean build-in-container

SRC=$(wildcard *.c)
HASH?=$(shell git ls-tree HEAD -- ../$(notdir $(CURDIR)) | awk '{print $$3}')

all: sbin/detect

build-in-container:
	docker build -t detect:$(HASH) -f Dockerfile .

sbin/detect: $(SRC)
	mkdir -p sbin
	gcc -static -o "$@" -lpthread -O2 -Wall -Werror $^

clean:
	rm -rf sbin
