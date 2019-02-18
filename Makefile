TARGETS=echo_server echo_client

CC = gcc
LD = ld
OBJCOPY = objcopy
SIZE = size
CC_FLAGS = -g -Wall -pedantic# -std=gnu99
CL_FLAGS = #-lpthread
INC = .

SRC = ikcp.c \
	echo_server.c \
	echo_client.c
OBJS = $(addsuffix .o, $(basename $(SRC)))

all:$(TARGETS)

$(TARGETS):%:%.o ikcp.o
	$(CC) $(CC_FLAGS) -I. $< ikcp.o -o $@ $(CL_FLAGS)

$(OBJS):%.o:%.c $(INC) Makefile
	$(CC) $(CC_FLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGETS)

debug:
	@echo $(OBJS)
	$(warning is $(OBJS))
