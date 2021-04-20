CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
PROJECT = proj2

all: $(PROJECT)

$(PROJECT): $(PROJECT).c
	$(CC) $(CFLAGS) $(PROJECT).c -o $(PROJECT)