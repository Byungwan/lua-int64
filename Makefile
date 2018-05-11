LUA_VERSION = 5.2
LUA_INCL_DIR = /usr/include/lua$(LUA_VERSION)

TARGET = int64.so
#CFLAGS = -g -Wall -pedantic -fno-inline
CFLAGS = -O3 -Wall -pedantic -DNDEBUG
INT64_CFLAGS = -fpic -I$(LUA_INCL_DIR)
INT64_LDFLAGS = -shared
OBJS = lua_int64.o

.PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INT64_CFLAGS) -o $@ $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(INT64_LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)
