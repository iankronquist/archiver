CC=clang
CFLAGS= -std=c99 -ffreestanding -O2 -Wall -Wextra -Wno-incompatible-pointer-types -g

all: ar

ar: build
	$(CC) ar.c -o build/bin/ar $(CFLAGS)

test:
	./runtests.sh

build:
	mkdir -p build/bin
	mkdir -p tests/bin

clean:
	rm -rf build
	rm -rf tests