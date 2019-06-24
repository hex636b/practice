
CFLAGS :=  -g
LD_FLAGS := 

CC := gcc

SERVER := server
CLIENT := client

SOURCES = $(shell find . -name "*.c")


.PHONY: clean
all: $(SERVER) $(CLIENT)
	
%.o : %.c
	$(CC) -c $^ -o $@

clean:
	-rm $(SERVER) $(CLIENT)

	
