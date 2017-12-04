PROJECT=amaze
SOURCES=$(shell find . -maxdepth 1 -type f -name '*cpp') 
LIB_TYPE=
INCPATHS=
LIBPATHS=
LDFLAGS=-lboost_system -lboost_filesystem \
		-lsfml-graphics -lsfml-window -lsfml-system
CFLAGS=-std=c++14 -c -Wall -Wextra -Werror -Wpedantic
CC=g++

# Automatic generation of some important lists
OBJECTS=$(SOURCES:.cpp=.o)
	INCFLAGS=$(foreach TMP,$(INCPATHS),-I$(TMP))
	LIBFLAGS=$(foreach TMP,$(LIBPATHS),-L$(TMP))

	BINARY=$(PROJECT)

all: CFLAGS += -O3
all: $(SOURCES) $(BINARY)

debug: CFLAGS += -O0 -g -D_DEBUG
debug: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $(LIBFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(INCFLAGS) $(CFLAGS) $< -o $@

distclean: clean
	rm -f $(BINARY)

clean:
	rm -f $(OBJECTS)

