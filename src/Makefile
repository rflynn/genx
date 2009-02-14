# ex: set ff=dos ts=8 noet:
# $Id$
#

CFLAGS = -W -Wall -pedantic -std=gnu99 -Wno-unused -ggdb -Os -DDEBUG
LDFLAGS = -lm
BIN = genx
ALL = genx
OBJ = rnd.o x86.o gen.o genx.o

all: $(ALL)

genx: $(OBJ)

clean:
	$(RM) $(ALL) $(OBJ) cscope.out


