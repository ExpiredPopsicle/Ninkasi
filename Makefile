
CFLAGS := -g -Wall
LDFLAGS := -g -Wall

SOURCES := $(shell find src -iname "*.c")
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

a : $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

clean :
	-rm $(OBJECTS)
