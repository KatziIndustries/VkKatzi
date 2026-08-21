CC = gcc
CFLAGS = -shared -fPIC -Wall -Wextra

VERSION = 0.1.0

LIB = libvkkatzi.so
LIB_VERSIONED = $(LIB).$(VERSION)
LIB_BUILD = build/$(LIB_VERSIONED)

LIB_SDL3 = libvkkatzi_SDL3.so
LIB_SDL3_VERSIONED = $(LIB_SDL3).$(VERSION)
LIB_SDL3_BUILD = build/$(LIB_SDL3_VERSIONED)

.PHONY: all install clean

all: $(LIB_BUILD)

$(LIB_BUILD):
	mkdir -p build
	$(CC) $(CFLAGS) src/* -lm -lvulkan -o $@

$(LIB_SDL3_BUILD): $(LIB_BUILD)
	mkdir -p build
	$(CC) $(CFLAGS) SDL3/* -lSDL3 -o $@

install: $(LIB_BUILD) $(LIB_SDL3_BUILD)
	sudo cp $(LIB_BUILD) /usr/lib/$(LIB_VERSIONED)
	sudo rm -f /usr/lib/$(LIB)
	sudo ln -s /usr/lib/$(LIB_VERSIONED) /usr/lib/$(LIB)

	sudo cp $(LIB_SDL3_BUILD) /usr/lib/$(LIB_SDL3_VERSIONED)
	sudo rm -f /usr/lib/$(LIB_SDL3)
	sudo ln -s /usr/lib/$(LIB_SDL3_VERSIONED) /usr/lib/$(LIB_SDL3)

	sudo ldconfig

clean:
	rm -rf build