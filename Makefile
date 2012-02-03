CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-llua
SOURCES=src/luna.c src/logger.c src/state.c src/config.c src/bot.c \
		src/net.c src/util.c src/irc.c src/handlers.c src/linked_list.c \
		src/channel.c src/lua_api/lua_manager.c src/lua_api/lua_api_functions.c\
		src/lua_api/lua_util.c src/lua_api/lua_user.c src/lua_api/lua_script.c \
		src/lua_api/lua_self.c src/lua_api/lua_channel.c src/user.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=luna

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
