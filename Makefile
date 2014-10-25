CC=clang
CFLAGS=  -O0 -Wall -Wextra -Wno-incompatible-pointer-types -g

all: ar

ar: build
	$(CC) ar.c -o ar $(CFLAGS)

test:
	./runtests.sh

build:
	mkdir -p build/bin
	mkdir -p tests/bin

clean:
	rm -rf build
	rm -rf tests
	rm -f test.ar
