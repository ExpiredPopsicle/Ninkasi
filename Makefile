
CFLAGS := -g -Wall

SOURCES := $(shell find src -iname "*.c")
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

a : $(OBJECTS)
	$(CC) -g -Wall $^ -o $@

clean :
	-rm $(OBJECTS)
