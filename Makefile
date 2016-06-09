
CFLAGS := -pedantic -pthread -lcrypt
LDFLAGS := -pthread -lcrypt
OPTS := -DDEBUG

source_files := $(wildcard src/*.c)
source_heads := $(wildcard src/*.h)

obj_files := $(patsubst src/%.c, \
	build/obj/%.o, $(source_files))

binary := ttydm

all: $(obj_files)
	@ld -r $+ -o build/$(binary).o
	@gcc $(LDFLAGS) -o build/$(binary) build/$(binary).o
	@rm build/$(binary).o

build/obj/%.o: src/%.c
	@mkdir -p $(shell dirname $@)
	gcc -c $< -o $@ $(CFLAGS) $(OPTS)

clean:
	@rm -r build

