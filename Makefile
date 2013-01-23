CompileBase=g++-4.7 -Wall -Wextra -pedantic --std=c++11 -ggdb -O0 -c -fPIC -fpic `pkg-config --cflags lua5.2` `pkg-config --cflags cairo`
LinkBase=g++-4.7 
LDFLAGS=`pkg-config --libs lua5.2` `pkg-config --libs cairo`

all: build/luacairo build/cairo.so

build: 
	mkdir build

build/binding.o: build app/binding.cxx
	$(CompileBase) app/binding.cxx -o build/binding.o

build/standalone.o: build app/standalone.cxx
	$(CompileBase) app/standalone.cxx -o build/standalone.o

build/luacairo: build build/standalone.o build/binding.o
	$(LinkBase) build/standalone.o build/binding.o $(LDFLAGS) -o build/luacairo

build/cairo.so: build build/binding.o
	$(LinkBase) -shared build/binding.o $(LDFLAGS) -o build/cairo.so

