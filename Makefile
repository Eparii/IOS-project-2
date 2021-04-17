CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
PROJECT = proj2

all: $(PROJECT)

$(PROJECT): $(PROJECT).c
	$(CC) $(CFLAGS) $(PROJECT).c -o $(PROJECT)