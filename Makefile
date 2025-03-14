.DEFAULT_GOAL := all
.PHONY: all install clean

all:
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -B build -S .
	cmake --build build

install:
	cmake --install build

clean:
	rm -rf build/