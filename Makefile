.PHONY: all tester emu

tester:
	(cd tester && mkdir -p build && cd build && cmake .. && make)

emu:
	(cd src && mkdir -p build && cd build && cmake .. && make)

all: tester emu
