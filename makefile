CC       = icc
CFLAGS   = -std=c99 -Wall -qopenmp -O3 #-w3 #-g
CPPFLAGS = -D_GNU_SOURCE -UDEBUG

# Source/Object Files
SRC_DIR = .
INC_DIR = -I$(SRC_DIR)

HEADERS = $(wildcard $(SRC_DIR)/*.h)
SRCS    = $(wildcard $(SRC_DIR)/*.c)
OBJS    = $(patsubst %.c,%.o,$(SRCS))

tackle: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.SUFFIXES: .c .o
$(OBJS): $(HEADERS)
.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INC_DIR) -c $< -o $@

clean:
	rm -rf $(OBJS) tackle

.PHONY: all clean
