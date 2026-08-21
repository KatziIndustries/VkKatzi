CC = gcc
CFLAGS = -shared -fPIC -Wall -Wextra

FILES = logger.c src/*
LIBS = -lvulkan -lm
VERSION = "0.1.0"

main: main.c
	mkdir -p build
	$(CC) $(CFLAGS) $(FILES) $(LIBS) -o build/libvkkatzi.so.$(VERSION)

clean:
	rm -rf build


# gcc -shared -fPIC sdl.c -o build/libvkkatzi_sdl3.so.$VERSION -Lbuild -lvkkatzi -lSDL3