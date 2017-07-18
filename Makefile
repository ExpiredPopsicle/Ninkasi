
CFLAGS := -g -Wall
LDFLAGS := -g -Wall

SOURCES := $(shell find src -iname "*.c")
HEADERS := $(shell find src -iname "*.h")
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $(filter %.c,$^) -o $@

a : $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

test : a
	valgrind ./a

clean :
	-rm $(OBJECTS)
