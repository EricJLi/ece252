
# Makefile, ECE252  

CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flg 
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
LIB_UTIL = zutil.o crc.o
SRCS   = catpng.c crc.c zutil.c
OBJS   = catpng.o $(LIB_UTIL) 
OBJS2  = findpng.o

TARGETS= catpng.out findpng.out result.png

all: ${TARGETS}

catpng.out: $(OBJS) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

findpng.out: $(OBJS2)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 
